// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLLISTSIDES_HPP
#define MCPTOOLLISTSIDES_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief list_sides 工具：返回平台数据面板中可选的阵营（side）名称。
//! @note 阵营列表直接取自 UtColor 的颜色表，与 PlatformData 插件中“Side”下拉框的数据源一致。
//! @note Execute() 必须在 GUI 线程中调用。
class ListSidesTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;

private:
   //! 收集并排序全部阵营名；颜色表是 unordered_map，输出需要稳定顺序
   static std::vector<std::string> CollectSideNames();
};
} // namespace McpServer

#endif // MCPTOOLLISTSIDES_HPP
