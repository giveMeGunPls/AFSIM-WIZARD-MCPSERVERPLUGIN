// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolUpdateWaypoint.hpp"

#include <string>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace
{
//! 在航路点属性区间内查找关键字行
size_t FindAttributeLine(const std::vector<McpServer::ScenarioEdit::TextLine>& aLines,
                         size_t aFrom,
                         size_t aTo,
                         size_t aMinIndent,
                         const std::string& aKeyword)
{
   for (size_t i = aFrom; i < aTo; ++i)
   {
      if ((aLines[i].mIndent >= aMinIndent) && McpServer::ScenarioEdit::LineHasKeyword(aLines[i], aKeyword))
      {
         return i;
      }
   }
   return std::string::npos;
}
} // namespace

namespace McpServer
{
std::string UpdateWaypointTool::Name() const
{
   return "update_waypoint";
}

std::string UpdateWaypointTool::Title() const
{
   return "更新航路点";
}

std::string UpdateWaypointTool::Description() const
{
   return "按航线内从 0 开始的索引修改航路点：位置、高度、速度与标签均可选。"
          "只重写你提供的值。";
}

std::vector<ToolParameter> UpdateWaypointTool::Parameters() const
{
   return {
      {"index", "航线内从 0 开始的航路点索引。", true},
      {"platform", "拥有该航线的平台。scope 为 'platform' 时必填。", false},
      {"scope", "取值为 'platform'（默认）或 'global'。", false},
      {"route", "平台航线的航线名称，或全局航线名称。", false},
      {"latitude", "新的纬度。须同时提供经度。", false},
      {"longitude", "新的经度。须同时提供纬度。", false},
      {"altitude", "新的带单位高度，如 '8000 m'。", false},
      {"speed", "新的带单位速度，如 '250 m/s'。", false},
      {"label", "新的航路点标签。", false},
   };
}

ToolTextResult UpdateWaypointTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   size_t            index = 0;
   const std::string platform  = ScenarioEdit::Argument(aArguments, "platform");
   const std::string scope     = ScenarioEdit::Argument(aArguments, "scope");
   const std::string route     = ScenarioEdit::Argument(aArguments, "route");
   const std::string latitude  = ScenarioEdit::Argument(aArguments, "latitude");
   const std::string longitude = ScenarioEdit::Argument(aArguments, "longitude");
   const std::string altitude  = ScenarioEdit::Argument(aArguments, "altitude");
   const std::string speed     = ScenarioEdit::Argument(aArguments, "speed");
   const std::string label     = ScenarioEdit::Argument(aArguments, "label");

   if (!ScenarioEdit::ParseIndex(ScenarioEdit::Argument(aArguments, "index"), index))
   {
      result.mMessage = "'index' must be a non-negative integer.";
      return result;
   }
   if (latitude.empty() != longitude.empty())
   {
      result.mMessage = "Provide both 'latitude' and 'longitude', or neither.";
      return result;
   }
   if (latitude.empty() && altitude.empty() && speed.empty() && label.empty())
   {
      result.mMessage = "Provide at least one of 'latitude'/'longitude', 'altitude', 'speed', or 'label'.";
      return result;
   }

   std::string error;
   ScenarioEdit::RouteLocation location;
   if (!ScenarioEdit::LocateRouteBlock(platform, scope.empty() ? "platform" : scope, route, location, error))
   {
      result.mMessage = error;
      return result;
   }

   const std::vector<ScenarioEdit::TextLine> lines      = ScenarioEdit::SplitLines(location.mText, location.mBegin);
   const std::string                         indentUnit = ScenarioEdit::IndentUnit();

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

   const size_t       positionIndex  = positions[index];
   const size_t       waypointIndent = lines[positionIndex].mIndent;
   const size_t       nextWaypoint   = (index + 1 < positions.size()) ? positions[index + 1] : endRouteIndex;
   const std::string  attributeIndent(waypointIndent + indentUnit.size(), ' ');

   // 现有 position 行：position <lat> <lon> [altitude <alt>]
   const std::vector<std::string> tokens = ScenarioEdit::SplitTokens(lines[positionIndex].mText);
   std::string                    existingLatitude;
   std::string                    existingLongitude;
   std::string                    inlineAltitude;
   if (tokens.size() >= 3)
   {
      existingLatitude  = tokens[1];
      existingLongitude = tokens[2];
      if ((tokens.size() >= 5) && (tokens[3] == "altitude"))
      {
         inlineAltitude = tokens[4];
      }
   }

   std::vector<ScenarioEdit::Edit> edits;

   // 位置行需要重写的情况：改了经纬度，或只改行内高度
   const bool rewritePosition = !latitude.empty() || (!altitude.empty() && !inlineAltitude.empty());
   if (rewritePosition)
   {
      const std::string newLatitude  = latitude.empty() ? existingLatitude : latitude;
      const std::string newLongitude = latitude.empty() ? existingLongitude : longitude;
      const std::string newAltitude  = altitude.empty() ? inlineAltitude : altitude;

      std::string content = "position " + newLatitude + " " + newLongitude;
      if (!newAltitude.empty())
      {
         content += " altitude " + newAltitude;
      }
      edits.push_back({lines[positionIndex].mBegin + waypointIndent, lines[positionIndex].mContentEnd, content});
   }

   // 独立 altitude 行只在原本没有行内高度时才存在
   if (!altitude.empty() && inlineAltitude.empty())
   {
      const size_t altitudeIndex = FindAttributeLine(lines, positionIndex + 1, nextWaypoint, waypointIndent, "altitude");
      if (altitudeIndex != std::string::npos)
      {
         edits.push_back({lines[altitudeIndex].mBegin + lines[altitudeIndex].mIndent,
                          lines[altitudeIndex].mContentEnd,
                          "altitude " + altitude});
      }
      else
      {
         const size_t insertAt = ScenarioEdit::LineEndIncludingNewline(lines[positionIndex]) + 1;
         std::string  text     = attributeIndent + "altitude " + altitude + "\n";
         if (!lines[positionIndex].mHasNewline)
         {
            text.insert(0, "\n"); // position 行是文件最后一行时先补换行
         }
         edits.push_back({insertAt, insertAt - 1, text});
      }
   }

   if (!speed.empty())
   {
      const size_t speedIndex = FindAttributeLine(lines, positionIndex + 1, nextWaypoint, waypointIndent, "speed");
      if (speedIndex != std::string::npos)
      {
         edits.push_back({lines[speedIndex].mBegin + lines[speedIndex].mIndent,
                          lines[speedIndex].mContentEnd,
                          "speed " + speed});
      }
      else
      {
         const size_t insertAt = ScenarioEdit::LineEndIncludingNewline(lines[positionIndex]) + 1;
         std::string  text     = attributeIndent + "speed " + speed + "\n";
         if (!lines[positionIndex].mHasNewline)
         {
            text.insert(0, "\n");
         }
         edits.push_back({insertAt, insertAt - 1, text});
      }
   }

   if (!label.empty())
   {
      const bool hasLabel = (positionIndex > 0) && (lines[positionIndex - 1].mIndent == waypointIndent) &&
                            ScenarioEdit::LineHasKeyword(lines[positionIndex - 1], "label");
      if (hasLabel)
      {
         edits.push_back({lines[positionIndex - 1].mBegin + waypointIndent,
                          lines[positionIndex - 1].mContentEnd,
                          "label " + label});
      }
      else
      {
         edits.push_back({lines[positionIndex].mBegin,
                          lines[positionIndex].mBegin - 1,
                          std::string(waypointIndent, ' ') + "label " + label + "\n"});
      }
   }

   if (!ScenarioEdit::ApplyEdits(location.mSource, edits, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Updated waypoint " + std::to_string(index) + " of the route.";
   return result;
}
} // namespace McpServer
