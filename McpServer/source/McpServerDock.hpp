// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPSERVERDOCK_HPP
#define MCPSERVERDOCK_HPP

#include <atomic>
#include <memory>
#include <thread>

#include <QDockWidget>
#include <QString>

class QLabel;
class QPushButton;
class QSpinBox;

namespace McpServer
{
class ToolServer;

//! @brief MCP 服务面板：启动/停止 Streamable HTTP 服务。
//! @note 服务运行在独立线程，工具对场景的修改一律投递回 GUI 线程执行。
class McpServerDock : public QDockWidget
{
   Q_OBJECT

public:
   explicit McpServerDock(QWidget* aParent = nullptr);
   ~McpServerDock() override;

signals:
   //! 服务线程完成启动后发出，由 GUI 线程更新界面状态
   void StartupResultReady(bool aSuccess, const QString& aMessage);

private slots:
   void OnToggleServer();

private:
   void StartServer();
   void StopServer();
   void RunServer();
   void OnStartupResult(bool aSuccess, const QString& aMessage);
   void SetStatus(const QString& aText);
   void UpdateUi();

   QString EndpointUrl() const;

   QSpinBox*    mPortSpin;
   QLabel*      mEndpointLabel;
   QLabel*      mStatusLabel;
   QPushButton* mToggleButton;

   //! 通知服务线程放弃等待 GUI 线程，避免停止服务时卡住
   std::shared_ptr<std::atomic<bool>> mAbortFlag;
   std::unique_ptr<ToolServer>        mToolServer;
   std::thread                        mServerThread;
};
} // namespace McpServer

#endif // MCPSERVERDOCK_HPP
