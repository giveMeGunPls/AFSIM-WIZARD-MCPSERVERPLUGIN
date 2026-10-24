// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLSETPLATFORMATTRIBUTES_HPP
#define MCPTOOLSETPLATFORMATTRIBUTES_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief set_platform_attributes 工具：就地修改平台定义中的顶层属性行。
//! @note Execute() 必须在 GUI 线程中调用。
class SetPlatformAttributesTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;

private:
   //! 一个待修改的属性；mKeyword 用于定位脚本关键字
   struct Attribute
   {
      std::string mKeyword;
      std::string mValue;
   };

   //! 参数转换与语义校验
   static bool ParseRequest(const ToolArguments& aArguments,
                            std::string&         aName,
                            std::vector<Attribute>& aAttributes,
                            std::string&         aError);

   //! 属性值不能包含换行，否则会破坏脚本结构
   static bool IsSingleLine(const std::string& aValue);
};
} // namespace McpServer

#endif // MCPTOOLSETPLATFORMATTRIBUTES_HPP
