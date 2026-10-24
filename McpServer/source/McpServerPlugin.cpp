// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpServerPlugin.hpp"

#include "McpServerDock.hpp"
#include "WkfEnvironment.hpp"
#include "WkfMainWindow.hpp"

WKF_PLUGIN_DEFINE_SYMBOLS(McpServer::Plugin,
                          "MCP Server",
                          "Exposes Wizard scenario editing as MCP tools over streamable HTTP.",
                          "wizard");

namespace McpServer
{
Plugin::Plugin(const QString& aPluginName, const size_t aUniqueId)
   : wizard::Plugin(aPluginName, aUniqueId)
{
   mDockPtr = new McpServerDock(wkfEnv.GetMainWindow());
   wkfEnv.GetMainWindow()->addDockWidget(Qt::RightDockWidgetArea, mDockPtr);
}
} // namespace McpServer
