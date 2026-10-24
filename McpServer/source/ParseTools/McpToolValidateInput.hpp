// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLVALIDATEINPUT_HPP
#define MCPTOOLVALIDATEINPUT_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief validate_input 工具：用项目的语法定义把一段 AFSIM 输入块单独解析一遍。
//! @note Execute() 必须在 GUI 线程中调用；输入文本不会写入任何文件。
class ValidateInputTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;
};
} // namespace McpServer

#endif // MCPTOOLVALIDATEINPUT_HPP
