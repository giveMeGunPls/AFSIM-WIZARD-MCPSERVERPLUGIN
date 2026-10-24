// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpServerDock.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include <QFormLayout>
#include <QLabel>
#include <QMetaObject>
#include <QObject>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "FileTools/McpFileTools.hpp"
#include "McpScenarioTools.hpp"
#include "McpToolRegistry.hpp"
#include "McpToolServer.hpp"

namespace
{
constexpr int kDefaultPort = 8765;
constexpr int kGuiWaitMs   = 5000; //!< 服务线程等待 GUI 线程完成工具动作的上限
constexpr int kAbortPollMs = 100;  //!< 停止服务时让等待中的工具请求尽快退出

//! 一次 GUI 线程动作的共享状态：GUI 线程写入结果，服务线程等待读取
template<typename TResult>
class GuiAction
{
public:
   explicit GuiAction(std::function<TResult()> aAction)
      : mAction(std::move(aAction))
      , mResult(std::make_shared<TResult>())
      , mDone(std::make_shared<bool>(false))
      , mMutex(std::make_shared<std::mutex>())
      , mCompletion(std::make_shared<std::condition_variable>())
   {
   }

   //! 由 QMetaObject::invokeMethod 在 GUI 线程中调用
   void operator()()
   {
      const TResult localResult = mAction();
      {
         std::lock_guard<std::mutex> lock(*mMutex);
         *mResult = localResult;
         *mDone   = true;
      }
      mCompletion->notify_all();
   }

   //! 在调用线程等待动作完成；中止或超时时返回默认构造的结果
   TResult Wait(const std::shared_ptr<std::atomic<bool>>& aAbortFlag) const
   {
      const std::chrono::steady_clock::time_point deadline =
         std::chrono::steady_clock::now() + std::chrono::milliseconds(kGuiWaitMs);

      std::unique_lock<std::mutex> lock(*mMutex);
      while (!*mDone)
      {
         if (aAbortFlag->load() || (std::chrono::steady_clock::now() >= deadline))
         {
            break;
         }
         mCompletion->wait_for(lock, std::chrono::milliseconds(kAbortPollMs));
      }
      return *mResult;
   }

private:
   std::function<TResult()>                 mAction;
   std::shared_ptr<TResult>                 mResult;
   std::shared_ptr<bool>                    mDone;
   std::shared_ptr<std::mutex>              mMutex;
   std::shared_ptr<std::condition_variable> mCompletion;
};

//! 把动作投递到 GUI 线程执行，并在当前线程等待结果
template<typename TResult>
TResult RunInGuiThread(const std::shared_ptr<std::atomic<bool>>& aAbortFlag,
                       QObject*                                 aContext,
                       std::function<TResult()>                 aAction)
{
   GuiAction<TResult> action(std::move(aAction));
   QMetaObject::invokeMethod(aContext, action, Qt::QueuedConnection);
   return action.Wait(aAbortFlag);
}

//! 启动结果转发器：服务线程调用，通过信号切回 GUI 线程
class StartupResultForwarder
{
public:
   explicit StartupResultForwarder(McpServer::McpServerDock* aDock)
      : mDock(aDock)
   {
   }

   void operator()(const McpServer::StartupResult& aResult) const
   {
      mDock->StartupResultReady(aResult.mSuccess, QString::fromStdString(aResult.mMessage));
   }

private:
   McpServer::McpServerDock* mDock;
};
} // namespace

namespace McpServer
{
McpServerDock::McpServerDock(QWidget* aParent)
   : QDockWidget(tr("MCP 服务"), aParent)
   , mPortSpin(new QSpinBox(this))
   , mEndpointLabel(new QLabel(this))
   , mStatusLabel(new QLabel(this))
   , mToggleButton(new QPushButton(tr("启动服务"), this))
{
   setObjectName(QStringLiteral("McpServerDock"));

   mPortSpin->setRange(1024, 65535);
   mPortSpin->setValue(kDefaultPort);

   mEndpointLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
   mStatusLabel->setWordWrap(true);

   QWidget*     content = new QWidget(this);
   QFormLayout* form    = new QFormLayout();
   form->addRow(tr("监听端口："), mPortSpin);
   form->addRow(tr("服务地址："), mEndpointLabel);

   QLabel* hint = new QLabel(tr("供外部 MCP 客户端连接的 Streamable HTTP 端点（POST/GET/DELETE）。"
                                "AFSIM Wizard 运行期间服务监听在 127.0.0.1 上。"),
                             content);
   hint->setWordWrap(true);

   QVBoxLayout* layout = new QVBoxLayout(content);
   layout->addLayout(form);
   layout->addWidget(mStatusLabel);
   layout->addWidget(mToggleButton);
   layout->addWidget(hint);
   layout->addStretch(1);
   setWidget(content);

   connect(mToggleButton, &QPushButton::clicked, this, &McpServerDock::OnToggleServer);
   connect(mPortSpin,
           static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
           this,
           &McpServerDock::UpdateUi);

   // 服务线程只发信号，界面状态一律在 GUI 线程更新
   connect(this, &McpServerDock::StartupResultReady, this, &McpServerDock::OnStartupResult, Qt::QueuedConnection);

   SetStatus(tr("已停止。"));
   UpdateUi();
}

McpServerDock::~McpServerDock()
{
   StopServer();
}

void McpServerDock::OnToggleServer()
{
   if (mServerThread.joinable())
   {
      StopServer();
   }
   else
   {
      StartServer();
   }
}

void McpServerDock::StartServer()
{
   if (mServerThread.joinable())
   {
      return;
   }

   mAbortFlag = std::make_shared<std::atomic<bool>>(false);

   // 注册表由服务线程与 GUI 线程共享，工具对象在服务启动前一次性登记完成
   std::shared_ptr<ToolRegistry> registry = std::make_shared<ToolRegistry>();
   RegisterScenarioTools(*registry);
   RegisterFileTools(*registry);

   // 按值捕获停止标志和注册表，避免服务线程在停止过程中读到已重置的成员
   const std::shared_ptr<std::atomic<bool>> abortFlag = mAbortFlag;
   GuiToolExecutor executor = [this, abortFlag, registry](const std::string& aToolName,
                                                         const ToolArguments& aArguments) {
      return RunInGuiThread<ToolTextResult>(abortFlag, this, [registry, aToolName, aArguments]() {
         const McpTool* tool = registry->Find(aToolName);
         if (!tool)
         {
            ToolTextResult result;
            result.mMessage = "Unknown tool: " + aToolName;
            return result;
         }
         return tool->Execute(aArguments);
      });
   };
   StartupReporter reporter = StartupResultForwarder(this);

   mToolServer.reset(new ToolServer(mPortSpin->value(), registry, std::move(executor), reporter));

   SetStatus(tr("正在启动：%1 ...").arg(EndpointUrl()));
   UpdateUi();

   mServerThread = std::thread(&McpServerDock::RunServer, this);
}

void McpServerDock::RunServer()
{
   mToolServer->Run();
}

void McpServerDock::StopServer()
{
   if (!mServerThread.joinable())
   {
      return;
   }

   SetStatus(tr("正在停止 ..."));
   if (mAbortFlag)
   {
      mAbortFlag->store(true);
   }
   if (mToolServer)
   {
      mToolServer->Stop();
   }

   mServerThread.join();

   mToolServer.reset();
   mAbortFlag.reset();

   SetStatus(tr("已停止。"));
   UpdateUi();
}

void McpServerDock::OnStartupResult(bool aSuccess, const QString& aMessage)
{
   // 启动过程中用户已经点击停止，此时线程已回收，不再更新状态
   if (!mServerThread.joinable())
   {
      return;
   }

   if (aSuccess)
   {
      SetStatus(tr("运行中：%1").arg(EndpointUrl()));
   }
   else
   {
      SetStatus(tr("启动失败：%1").arg(aMessage));
      mServerThread.join();
      mToolServer.reset();
      mAbortFlag.reset();
   }
   UpdateUi();
}

void McpServerDock::SetStatus(const QString& aText)
{
   mStatusLabel->setText(aText);
}

QString McpServerDock::EndpointUrl() const
{
   return QStringLiteral("http://127.0.0.1:%1/mcp").arg(mPortSpin->value());
}

void McpServerDock::UpdateUi()
{
   const bool active = mServerThread.joinable();
   mToggleButton->setText(active ? tr("停止服务") : tr("启动服务"));
   mPortSpin->setEnabled(!active);
   mEndpointLabel->setText(EndpointUrl());
}
} // namespace McpServer
