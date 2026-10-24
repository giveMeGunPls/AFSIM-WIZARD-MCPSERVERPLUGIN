// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPPARSETOOLS_HPP
#define MCPPARSETOOLS_HPP

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! 注册全部解析/校验工具
void RegisterParseTools(ToolRegistry& aRegistry);
} // namespace McpServer

#endif // MCPPARSETOOLS_HPP
