// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolServer.hpp"

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "cxxmcp/core/result.hpp"
#include "cxxmcp/peer.hpp"
#include "cxxmcp/protocol.hpp"
#include "cxxmcp/server.hpp"
#include "cxxmcp/service.hpp"

namespace
{
//! 传输层绑定失败会立即结束监听线程，用它区分“启动中”与“绑定失败”
constexpr std::chrono::milliseconds kBindSettleTime(200);
} // namespace

namespace McpServer
{
struct ToolServer::Impl
{
   Impl(int                                 aPort,
        std::shared_ptr<const ToolRegistry> aRegistry,
        GuiToolExecutor                     aExecutor,
        StartupReporter                     aReporter)
      : mPort(aPort)
      , mRegistry(std::move(aRegistry))
      , mExecutor(std::move(aExecutor))
      , mReporter(std::move(aReporter))
   {
   }

   void Report(bool aSuccess, const std::string& aMessage)
   {
      if (!mReporter)
      {
         return;
      }

      StartupResult result;
      result.mSuccess = aSuccess;
      result.mMessage = aMessage;
      mReporter(result);
   }

   int                                 mPort;
   std::shared_ptr<const ToolRegistry> mRegistry;
   GuiToolExecutor                     mExecutor;
   StartupReporter                     mReporter;

   std::mutex              mMutex;
   std::condition_variable mCv;
   bool                    mStopRequested = false;
};

ToolServer::ToolServer(int                                 aPort,
                       std::shared_ptr<const ToolRegistry> aRegistry,
                       GuiToolExecutor                     aExecutor,
                       StartupReporter                     aReporter)
   : mImpl(new Impl(aPort, std::move(aRegistry), std::move(aExecutor), std::move(aReporter)))
{
}

ToolServer::~ToolServer()
{
   Stop();
}

void ToolServer::Run()
{
   mcp::server::HttpTransportOptions options;
   options.listen_host = "127.0.0.1";
   options.listen_port = mImpl->mPort;
   options.path        = "/mcp";

   mcp::server::ServerBuilder builder;
   builder.name("afsim-wizard")
      .version("0.1.0")
      .instructions("Tools edit the scenario opened in AFSIM Wizard by appending script text. "
                    "Changes take effect once Wizard reparses the script.")
      .with_transport(std::make_unique<mcp::server::HttpTransport>(options));

   // 注册表决定对外暴露哪些工具，新增工具无需改动服务层
   mImpl->mRegistry->ApplyTo(builder, mImpl->mExecutor);

   mcp::core::Result<std::unique_ptr<mcp::server::Server>> built = builder.build();
   if (!built.has_value())
   {
      mImpl->Report(false, built.error().message);
      return;
   }

   mcp::core::Result<mcp::RunningService<mcp::RoleServer>> served =
      mcp::serve(mcp::ServerPeer(std::move(*built)));
   if (!served.has_value())
   {
      mImpl->Report(false, served.error().message);
      return;
   }

   mcp::RunningService<mcp::RoleServer> service = std::move(*served);
   service.wait_until_ready();

   // 端口被占用时监听线程会立刻退出，短暂等待后据此判断是否启动成功
   std::this_thread::sleep_for(kBindSettleTime);
   if (!service.running())
   {
      const mcp::core::Result<mcp::core::Unit> waited = service.wait();
      mImpl->Report(false, waited.has_value() ? "HTTP 监听在启动过程中停止。" : waited.error().message);
      return;
   }

   bool stopRequested = false;
   {
      std::lock_guard<std::mutex> lock(mImpl->mMutex);
      stopRequested = mImpl->mStopRequested;
   }
   if (stopRequested)
   {
      // 启动期间已收到停止请求，直接退出
      (void)service.stop();
      return;
   }
   mImpl->Report(true, std::string());

   {
      std::unique_lock<std::mutex> lock(mImpl->mMutex);
      while (!mImpl->mStopRequested)
      {
         mImpl->mCv.wait(lock);
      }
   }

   (void)service.stop();
}

void ToolServer::Stop()
{
   {
      std::lock_guard<std::mutex> lock(mImpl->mMutex);
      mImpl->mStopRequested = true;
   }
   mImpl->mCv.notify_all();
}
} // namespace McpServer
