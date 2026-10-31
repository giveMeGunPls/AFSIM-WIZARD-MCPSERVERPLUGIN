// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLLISTICONS_HPP
#define MCPTOOLLISTICONS_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief list_icons 工具：按分类返回模型数据库中全部可用的图标（icon）名称。
//! @note 数据源与 PlatformData 插件中“Icon”下拉框一致：VaModelDatabase 的模型定义，
//!       分类取 VaModelDefinition::GetCategories()；参考实现只展示有分类的图标，
//!       这里额外把没有分类的图标归入 uncategorized，以保证能返回全部图标资源。
//! @note Execute() 必须在 GUI 线程中调用。
class ListIconsTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;

private:
   //! 一组图标：分类名 + 组内图标名（均按名称升序）
   struct IconGroup
   {
      std::string              mCategory;
      std::vector<std::string> mNames;
   };

   //! 按分类归并模型数据库中的图标；未设置分类的图标归入 uncategorized 组
   static std::vector<IconGroup> CollectIconGroups();
};
} // namespace McpServer

#endif // MCPTOOLLISTICONS_HPP
