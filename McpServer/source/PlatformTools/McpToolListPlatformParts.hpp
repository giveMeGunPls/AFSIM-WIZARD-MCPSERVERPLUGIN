// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLLISTPLATFORMPARTS_HPP
#define MCPTOOLLISTPLATFORMPARTS_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief list_platform_parts 工具：列出平台定义中的部件（sensor/weapon/comm/processor/mover/fuel）。
//! @note Execute() 必须在 GUI 线程中调用。
class ListPlatformPartsTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;
};
} // namespace McpServer

#endif // MCPTOOLLISTPLATFORMPARTS_HPP
