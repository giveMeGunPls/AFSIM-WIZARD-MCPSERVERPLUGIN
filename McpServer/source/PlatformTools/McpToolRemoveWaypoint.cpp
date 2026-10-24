// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolRemoveWaypoint.hpp"

#include <string>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace McpServer
{
std::string RemoveWaypointTool::Name() const
{
   return "remove_waypoint";
}

std::string RemoveWaypointTool::Title() const
{
   return "删除航路点";
}

std::string RemoveWaypointTool::Description() const
{
   return "从航线块中删除一个航路点（其标签、位置与属性），按从 0 开始的索引指定。";
}

std::vector<ToolParameter> RemoveWaypointTool::Parameters() const
{
   return {
      {"index", "航线内从 0 开始的航路点索引。", true},
      {"platform", "拥有该航线的平台。scope 为 'platform' 时必填。", false},
      {"scope", "取值为 'platform'（默认）或 'global'。", false},
      {"route", "平台航线的航线名称，或全局航线名称。", false},
   };
}

ToolTextResult RemoveWaypointTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   size_t            index = 0;
   const std::string platform = ScenarioEdit::Argument(aArguments, "platform");
   const std::string scope    = ScenarioEdit::Argument(aArguments, "scope");
   const std::string route    = ScenarioEdit::Argument(aArguments, "route");

   if (!ScenarioEdit::ParseIndex(ScenarioEdit::Argument(aArguments, "index"), index))
   {
      result.mMessage = "'index' must be a non-negative integer.";
      return result;
   }

   std::string error;
   ScenarioEdit::RouteLocation location;
   if (!ScenarioEdit::LocateRouteBlock(platform, scope.empty() ? "platform" : scope, route, location, error))
   {
      result.mMessage = error;
      return result;
   }

   const std::vector<ScenarioEdit::TextLine> lines = ScenarioEdit::SplitLines(location.mText, location.mBegin);

   size_t endRouteIndex = std::string::npos;
   if (!ScenarioEdit::FindEndRouteLine(lines, location.mStartIndent, endRouteIndex))
   {
      result.mMessage = "The route block is not terminated by 'end_route'.";
      return result;
   }

   const std::vector<size_t> positions = ScenarioEdit::FindWaypointPositions(lines, location.mStartIndent);
   if (index >= positions.size())
   {
      result.mMessage = "Waypoint index " + std::to_string(index) + " is out of range; the route has " +
                        std::to_string(positions.size()) + " waypoint(s).";
      return result;
   }

   const size_t positionIndex  = positions[index];
   const size_t waypointIndent = lines[positionIndex].mIndent;
   const size_t nextWaypoint   = (index + 1 < positions.size()) ? positions[index + 1] : endRouteIndex;

   // 紧邻 position 行上方的 label 行属于同一个航路点
   size_t firstIndex = positionIndex;
   if ((positionIndex > 0) && (lines[positionIndex - 1].mIndent == waypointIndent) &&
       ScenarioEdit::LineHasKeyword(lines[positionIndex - 1], "label"))
   {
      --firstIndex;
   }

   if (nextWaypoint == 0)
   {
      result.mMessage = "Unable to determine the extent of the waypoint.";
      return result;
   }

   const ScenarioEdit::Edit edit{lines[firstIndex].mBegin,
                                 ScenarioEdit::LineEndIncludingNewline(lines[nextWaypoint - 1]),
                                 std::string()};
   if (!ScenarioEdit::ApplyEdits(location.mSource, {edit}, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Removed waypoint " + std::to_string(index) + " from the route.";
   return result;
}
} // namespace McpServer
