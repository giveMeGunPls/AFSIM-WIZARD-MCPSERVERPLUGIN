// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpStartupTools.hpp"

#include <memory>

#include "McpToolAddStartupFile.hpp"
#include "McpToolGetCurrentStartupFile.hpp"
#include "McpToolGetStartupFiles.hpp"
#include "McpToolRemoveStartupFile.hpp"
#include "McpToolSetCurrentStartupFile.hpp"
#include "McpToolSetStartupFiles.hpp"

namespace McpServer
{
void RegisterStartupTools(ToolRegistry& aRegistry)
{
   aRegistry.Register(std::make_unique<GetStartupFilesTool>());
   aRegistry.Register(std::make_unique<GetCurrentStartupFileTool>());
   aRegistry.Register(std::make_unique<SetCurrentStartupFileTool>());
   aRegistry.Register(std::make_unique<AddStartupFileTool>());
   aRegistry.Register(std::make_unique<RemoveStartupFileTool>());
   aRegistry.Register(std::make_unique<SetStartupFilesTool>());
}
} // namespace McpServer
