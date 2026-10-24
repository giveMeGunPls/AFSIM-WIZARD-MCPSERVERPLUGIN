// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPSCENARIOQUERY_HPP
#define MCPSCENARIOQUERY_HPP

#include <string>

namespace McpServer
{
//! @brief 场景只读查询，供各工具复用。
//! @note 所有方法都必须在 GUI 线程中调用。
class ScenarioQuery
{
public:
   //! 默认的目标脚本文件（平台定义最多的文件，相对项目目录）；无可用文件时返回空串
   static std::string DefaultTargetFile();
};
} // namespace McpServer

#endif // MCPSCENARIOQUERY_HPP
