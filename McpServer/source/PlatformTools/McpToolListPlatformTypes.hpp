// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLLISTPLATFORMTYPES_HPP
#define MCPTOOLLISTPLATFORMTYPES_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

class WsfPProxy;

namespace McpServer
{
//! @brief list_platform_types 工具：返回当前场景的平台类型、已有平台和默认目标文件。
//! @note Execute() 必须在 GUI 线程中调用。
class ListPlatformTypesTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;

private:
   //! 收集基础类型树中某一类（如 platformType）的名字
   static void CollectBasicTypeNames(WsfPProxy* aProxy, const char* aCategory, std::vector<std::string>& aNames);

   //! 排序并去重，保证工具输出稳定
   static void SortUnique(std::vector<std::string>& aValues);

   //! 追加“标题（数量）+ 列表”段落
   static void AppendSection(std::string& aText, const char* aTitle, const std::vector<std::string>& aValues);
};
} // namespace McpServer

#endif // MCPTOOLLISTPLATFORMTYPES_HPP
