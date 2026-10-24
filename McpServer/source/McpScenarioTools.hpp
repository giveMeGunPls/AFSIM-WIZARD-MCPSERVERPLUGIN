// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPSCENARIOTOOLS_HPP
#define MCPSCENARIOTOOLS_HPP

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! 注册全部场景工具；新增工具时只需在这里补一行
void RegisterScenarioTools(ToolRegistry& aRegistry);
} // namespace McpServer

#endif // MCPSCENARIOTOOLS_HPP
