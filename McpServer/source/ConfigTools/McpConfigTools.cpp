// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpConfigTools.hpp"

#include <memory>

#include "ConfigTools/McpToolSetEndTime.hpp"
#include "ConfigTools/McpToolSetEventOutput.hpp"
#include "ConfigTools/McpToolSetEventPipe.hpp"
#include "ConfigTools/McpToolSetLogFile.hpp"

namespace McpServer
{
void RegisterConfigTools(ToolRegistry& aRegistry)
{
   // 全局仿真配置
   aRegistry.Register(std::make_unique<SetLogFileTool>());
   aRegistry.Register(std::make_unique<SetEventOutputTool>());
   aRegistry.Register(std::make_unique<SetEventPipeTool>());
   aRegistry.Register(std::make_unique<SetEndTimeTool>());
}
} // namespace McpServer
