// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPCONFIGTOOLS_HPP
#define MCPCONFIGTOOLS_HPP

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! 登记日志、事件记录、事件回放与仿真时长等全局配置工具
void RegisterConfigTools(ToolRegistry& aRegistry);
} // namespace McpServer

#endif // MCPCONFIGTOOLS_HPP
