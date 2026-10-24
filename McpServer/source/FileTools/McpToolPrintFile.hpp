// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLPRINTFILE_HPP
#define MCPTOOLPRINTFILE_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief print_file 工具：打印文件，或把文件输出为 PDF。
//! @note Execute() 必须在 GUI 线程中调用。
class PrintFileTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;
};
} // namespace McpServer

#endif // MCPTOOLPRINTFILE_HPP
