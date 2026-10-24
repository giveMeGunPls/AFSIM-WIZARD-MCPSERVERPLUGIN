// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpFileTools.hpp"

#include <memory>

#include "McpToolCloseProject.hpp"
#include "McpToolExportProject.hpp"
#include "McpToolNewFile.hpp"
#include "McpToolNewProject.hpp"
#include "McpToolOpenFile.hpp"
#include "McpToolPrintFile.hpp"
#include "McpToolSaveAll.hpp"
#include "McpToolSaveFile.hpp"
#include "McpToolSaveFileAs.hpp"
#include "McpToolSaveProject.hpp"

namespace McpServer
{
void RegisterFileTools(ToolRegistry& aRegistry)
{
   aRegistry.Register(std::make_unique<NewFileTool>());
   aRegistry.Register(std::make_unique<OpenFileTool>());
   aRegistry.Register(std::make_unique<SaveFileTool>());
   aRegistry.Register(std::make_unique<SaveFileAsTool>());
   aRegistry.Register(std::make_unique<SaveAllTool>());
   aRegistry.Register(std::make_unique<PrintFileTool>());
   aRegistry.Register(std::make_unique<NewProjectTool>());
   aRegistry.Register(std::make_unique<CloseProjectTool>());
   aRegistry.Register(std::make_unique<SaveProjectTool>());
   aRegistry.Register(std::make_unique<ExportProjectTool>());
}
} // namespace McpServer
