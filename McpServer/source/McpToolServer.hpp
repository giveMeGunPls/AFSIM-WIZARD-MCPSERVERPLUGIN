// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLSERVER_HPP
#define MCPTOOLSERVER_HPP

#include <functional>
#include <memory>
#include <string>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! 启动结果；mSuccess 为 false 时 mMessage 说明失败原因
struct StartupResult
{
   bool        mSuccess = false;
   std::string mMessage;
};

//! 启动结果回调，由服务线程触发，实现方需自行切回 GUI 线程
using StartupReporter = std::function<void(const StartupResult&)>;

//! @brief 基于 cxxmcp 的 Streamable HTTP MCP 服务，把注册表内的工具暴露给外部客户端。
//! @note Run() 会阻塞直到 Stop() 被调用，因此调用方应把 Run() 放到独立线程中执行。
class ToolServer
{
public:
   //! @param aRegistry 工具注册表，决定对外暴露哪些工具；服务运行期间必须保持有效
   //! @param aExecutor 工具执行入口，负责切回 GUI 线程并返回执行结果
   ToolServer(int                                 aPort,
              std::shared_ptr<const ToolRegistry> aRegistry,
              GuiToolExecutor                     aExecutor,
              StartupReporter                     aReporter);
   ~ToolServer();

   ToolServer(const ToolServer&) = delete;
   ToolServer& operator=(const ToolServer&) = delete;

   //! 阻塞运行，直到 Stop() 被调用或启动失败
   void Run();

   //! 请求停止，可从任意线程调用
   void Stop();

private:
   struct Impl;
   std::unique_ptr<Impl> mImpl;
};
} // namespace McpServer

#endif // MCPTOOLSERVER_HPP
