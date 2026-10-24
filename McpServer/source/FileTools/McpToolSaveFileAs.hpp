// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLSAVEFILEAS_HPP
#define MCPTOOLSAVEFILEAS_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief save_file_as 工具：把当前文件内容另存为新的文件。
//! @note Execute() 必须在 GUI 线程中调用。
class SaveFileAsTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;
};
} // namespace McpServer

#endif // MCPTOOLSAVEFILEAS_HPP
