// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPFILETOOLS_HPP
#define MCPFILETOOLS_HPP

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! 登记 FileTools 目录下的全部文件操作工具。
void RegisterFileTools(ToolRegistry& aRegistry);
} // namespace McpServer

#endif // MCPFILETOOLS_HPP
