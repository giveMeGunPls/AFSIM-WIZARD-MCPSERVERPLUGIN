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
#include "PlatformTools/McpToolAddPlatformPart.hpp"
#include "PlatformTools/McpToolAddRoute.hpp"
#include "PlatformTools/McpToolAddWaypoint.hpp"
#include "PlatformTools/McpToolAddZone.hpp"
#include "PlatformTools/McpToolClonePlatform.hpp"
#include "PlatformTools/McpToolDeletePlatform.hpp"
#include "PlatformTools/McpToolFindPlatform.hpp"
#include "PlatformTools/McpToolFindZone.hpp"
#include "PlatformTools/McpToolListPlatformParts.hpp"
#include "PlatformTools/McpToolListPlatformPartTypes.hpp"
#include "PlatformTools/McpToolListPlatformTypes.hpp"
#include "PlatformTools/McpToolRemovePlatformPart.hpp"
#include "PlatformTools/McpToolRemoveRoute.hpp"
#include "PlatformTools/McpToolRemoveWaypoint.hpp"
#include "PlatformTools/McpToolRemoveZone.hpp"
#include "PlatformTools/McpToolSetPlatformAttributes.hpp"
#include "PlatformTools/McpToolUpdateWaypoint.hpp"

namespace McpServer
{
void RegisterScenarioTools(ToolRegistry& aRegistry)
{
   // 平台查询与编辑
   aRegistry.Register(std::make_unique<AddPlatformTool>());
   aRegistry.Register(std::make_unique<ListPlatformTypesTool>());
   aRegistry.Register(std::make_unique<DeletePlatformTool>());
   aRegistry.Register(std::make_unique<ClonePlatformTool>());
   aRegistry.Register(std::make_unique<FindPlatformTool>());
   aRegistry.Register(std::make_unique<SetPlatformAttributesTool>());

   // 平台部件
   aRegistry.Register(std::make_unique<ListPlatformPartTypesTool>());
   aRegistry.Register(std::make_unique<ListPlatformPartsTool>());
   aRegistry.Register(std::make_unique<AddPlatformPartTool>());
   aRegistry.Register(std::make_unique<RemovePlatformPartTool>());

   // 航线与航路点
   aRegistry.Register(std::make_unique<AddRouteTool>());
   aRegistry.Register(std::make_unique<AddWaypointTool>());
   aRegistry.Register(std::make_unique<UpdateWaypointTool>());
   aRegistry.Register(std::make_unique<RemoveWaypointTool>());
   aRegistry.Register(std::make_unique<RemoveRouteTool>());

   // 区域
   aRegistry.Register(std::make_unique<AddZoneTool>());
   aRegistry.Register(std::make_unique<FindZoneTool>());
   aRegistry.Register(std::make_unique<RemoveZoneTool>());
}
} // namespace McpServer
