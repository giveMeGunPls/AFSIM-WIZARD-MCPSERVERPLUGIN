// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPSERVERPLUGIN_HPP
#define MCPSERVERPLUGIN_HPP

#include <QPointer>

#include "Plugin.hpp"

namespace McpServer
{
class McpServerDock;

//! @brief 把 Wizard 的场景编辑能力以 MCP 工具的形式通过 HTTP 暴露给外部客户端。
//! @note 插件只负责 MCP 服务与界面，场景数据仍由代理树/解析器维护。
class Plugin : public wizard::Plugin
{
   Q_OBJECT

public:
   Plugin(const QString& aPluginName, const size_t aUniqueId);
   ~Plugin() override = default;

private:
   QPointer<McpServerDock> mDockPtr;
};

} // namespace McpServer

#endif // MCPSERVERPLUGIN_HPP
