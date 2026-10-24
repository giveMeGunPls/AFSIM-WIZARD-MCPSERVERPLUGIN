// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolAddWaypoint.hpp"

#include <string>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace McpServer
{
std::string AddWaypointTool::Name() const
{
   return "add_waypoint";
}

std::string AddWaypointTool::Title() const
{
   return "添加航路点";
}

std::string AddWaypointTool::Description() const
{
   return "在航线块末尾追加一个航路点（标签、位置与可选的速度）。"
          "该航线属于某个平台（scope 'platform'），或是具名的全局航线（scope 'global'）。";
}

std::vector<ToolParameter> AddWaypointTool::Parameters() const
{
   return {
      {"latitude", "AFSIM 格式的纬度，如 '27:52:46.72'。", true},
      {"longitude", "AFSIM 格式的经度，如 '118:01:01.03'。", true},
      {"platform", "拥有该航线的平台。scope 为 'platform' 时必填。", false},
      {"scope", "取值为 'platform'（默认）或 'global'。", false},
      {"route", "平台航线的航线名称，或全局航线名称。", false},
      {"altitude", "带单位的航路点高度，如 '8000 m'。", false},
      {"speed", "带单位的航路点速度，如 '250 m/s'。", false},
      {"label", "可选的航路点标签。", false},
   };
}

ToolTextResult AddWaypointTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string latitude  = ScenarioEdit::Argument(aArguments, "latitude");
   const std::string longitude = ScenarioEdit::Argument(aArguments, "longitude");
   const std::string platform  = ScenarioEdit::Argument(aArguments, "platform");
   const std::string scope     = ScenarioEdit::Argument(aArguments, "scope");
   const std::string route     = ScenarioEdit::Argument(aArguments, "route");
   const std::string altitude  = ScenarioEdit::Argument(aArguments, "altitude");
   const std::string speed     = ScenarioEdit::Argument(aArguments, "speed");
   const std::string label     = ScenarioEdit::Argument(aArguments, "label");

   std::string error;
   const bool  global = (scope == "global");
   ScenarioEdit::RouteLocation location;
   if (!ScenarioEdit::LocateRouteBlock(platform, scope.empty() ? "platform" : scope, route, location, error))
   {
      result.mMessage = error;
      return result;
   }

   const std::vector<ScenarioEdit::TextLine> lines      = ScenarioEdit::SplitLines(location.mText, location.mBegin);
   const std::string                         indentUnit = ScenarioEdit::IndentUnit();
   const std::string waypointIndent = std::string(location.mStartIndent + indentUnit.size(), ' ');

   size_t endRouteIndex = std::string::npos;
   if (!ScenarioEdit::FindEndRouteLine(lines, location.mStartIndent, endRouteIndex))
   {
      result.mMessage = "The route block is not terminated by 'end_route'.";
      return result;
   }

   // 平台航线的首个航路点必须带速度，否则 Wizard 无法计算航线；与手工编辑一致，缺省补 500 mph
   std::string effectiveSpeed = speed;
   if (!global && effectiveSpeed.empty() &&
       ScenarioEdit::FindWaypointPositions(lines, location.mStartIndent).empty())
   {
      effectiveSpeed = "500 mph";
   }

   const std::string block =
      ScenarioEdit::BuildWaypointText(waypointIndent, indentUnit, label, latitude, longitude, altitude, effectiveSpeed);

   const ScenarioEdit::Edit edit{lines[endRouteIndex].mBegin, lines[endRouteIndex].mBegin - 1, block};
   if (!ScenarioEdit::ApplyEdits(location.mSource, {edit}, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Added a waypoint to the route.\n\n" + block;
   return result;
}
} // namespace McpServer
