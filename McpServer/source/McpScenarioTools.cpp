// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpScenarioTools.hpp"

#include <memory>

#include "PlatformTools/McpToolAddPlatform.hpp"
#include "PlatformTools/McpToolListPlatformTypes.hpp"

namespace McpServer
{
void RegisterScenarioTools(ToolRegistry& aRegistry)
{
   aRegistry.Register(std::make_unique<AddPlatformTool>());
   aRegistry.Register(std::make_unique<ListPlatformTypesTool>());
}
} // namespace McpServer
