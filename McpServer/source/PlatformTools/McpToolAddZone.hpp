// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLADDZONE_HPP
#define MCPTOOLADDZONE_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief add_zone 工具：向脚本文件追加区域定义。
//! @note Execute() 必须在 GUI 线程中调用。
class AddZoneTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;

private:
   //! 把 "lat lon; lat lon" 形式的点串展开成脚本中的 point 行
   static bool AppendPoints(std::string& aZoneDef, const std::string& aIndent, const std::string& aPoints, std::string& aError);
};
} // namespace McpServer

#endif // MCPTOOLADDZONE_HPP
