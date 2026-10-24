// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLNEWFILE_HPP
#define MCPTOOLNEWFILE_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief new_file 工具：在项目中新建文本文件并打开编辑器。
//! @note Execute() 必须在 GUI 线程中调用。
class NewFileTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;
};
} // namespace McpServer

#endif // MCPTOOLNEWFILE_HPP
