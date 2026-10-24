// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPSTARTUPTOOLS_HPP
#define MCPSTARTUPTOOLS_HPP

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! 登记 StartupTools 目录下的全部启动文件操作工具。
void RegisterStartupTools(ToolRegistry& aRegistry);
} // namespace McpServer

#endif
