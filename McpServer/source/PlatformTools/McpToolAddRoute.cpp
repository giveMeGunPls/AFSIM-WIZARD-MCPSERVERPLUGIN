// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolAddRoute.hpp"

#include <string>
#include <vector>

#include "FileTools/McpFileToolSupport.hpp"
#include "McpScenarioEditSupport.hpp"
#include "McpScenarioQuery.hpp"
#include "Project.hpp"
#include "ProjectWorkspace.hpp"
#include "TextSource.hpp"
#include "WsfPM_Root.hpp"
#include "WsfPProxy.hpp"
#include "WsfPProxyObjectMap.hpp"

namespace McpServer
{
std::string AddRouteTool::Name() const
{
   return "add_route";
}

std::string AddRouteTool::Title() const
{
   return "添加航线";
}

std::string AddRouteTool::Description() const
{
   return "添加航线块。scope 为 'platform'（默认）时，向平台定义中插入一条空航线；"
          "scope 为 'global' 时，把具名全局航线追加到脚本文件。"
          "可同时创建第一个航路点。";
}

std::vector<ToolParameter> AddRouteTool::Parameters() const
{
   return {
      {"platform", "接收该航线的平台。scope 为 'platform' 时必填。", false},
      {"scope", "取值为 'platform'（默认）或 'global'。", false},
      {"route", "全局航线名称。scope 为 'global' 时必填。", false},
      {"file",
       "全局航线的目标脚本文件，相对于项目目录。缺省使用包含平台定义最多的文件。",
       false},
      {"latitude", "可选的首个航路点纬度。须同时提供经度。", false},
      {"longitude", "可选的首个航路点经度。须同时提供纬度。", false},
      {"altitude", "首个航路点的高度，如 '8000 m'。", false},
      {"speed", "首个航路点的速度，如 '250 m/s'。", false},
   };
}

ToolTextResult AddRouteTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string platform = ScenarioEdit::Argument(aArguments, "platform");
   const std::string scope    = ScenarioEdit::Argument(aArguments, "scope");
   const std::string route    = ScenarioEdit::Argument(aArguments, "route");
   const std::string file     = ScenarioEdit::Argument(aArguments, "file");
   const std::string latitude = ScenarioEdit::Argument(aArguments, "latitude");
   const std::string longitude = ScenarioEdit::Argument(aArguments, "longitude");
   const std::string altitude = ScenarioEdit::Argument(aArguments, "altitude");
   const std::string speed    = ScenarioEdit::Argument(aArguments, "speed");

   std::string error;
   if (!ScenarioEdit::RequireParsedProject(error))
   {
      result.mMessage = error;
      return result;
   }

   if (latitude.empty() != longitude.empty())
   {
      result.mMessage = "Provide both 'latitude' and 'longitude', or neither.";
      return result;
   }

   const std::string indentUnit = ScenarioEdit::IndentUnit();
   const bool        global     = (!scope.empty() && (scope == "global"));

   if (global)
   {
      if (route.empty())
      {
         result.mMessage = "A global route requires 'route' (its name).";
         return result;
      }
      if (WsfPM_Root(wizard::Project::Instance()->Proxy()).routes().Find(route).IsValid())
      {
         result.mMessage = "Global route '" + route + "' already exists.";
         return result;
      }

      const std::string relativeFile = file.empty() ? ScenarioQuery::DefaultTargetFile() : file;
      if (relativeFile.empty())
      {
         result.mMessage = "No target script file is available in this project; pass 'file' explicitly.";
         return result;
      }

      std::string pathError;
      const std::string absoluteFile = FileTools::ResolveProjectPath(relativeFile, pathError);
      if (absoluteFile.empty())
      {
         result.mMessage = pathError;
         return result;
      }

      wizard::TextSource* source = wizard::ProjectWorkspace::Instance()->FindSource(absoluteFile, false, false);
      if (!source)
      {
         result.mMessage = "Unable to find the target script file: " + relativeFile;
         return result;
      }

      std::string block = "route " + route + "\n";
      block += ScenarioEdit::BuildWaypointText(indentUnit, indentUnit, "", latitude, longitude, altitude, speed);
      block += "end_route\n";

      if (!ScenarioEdit::AppendText(source, "\n" + block, error))
      {
         result.mMessage = error;
         return result;
      }

      result.mSuccess = true;
      result.mMessage = "Created global route '" + route + "' in '" + relativeFile + "'.\n\n" + block;
      return result;
   }

   if (platform.empty())
   {
      result.mMessage = "A platform route requires 'platform'.";
      return result;
   }

   WsfPProxyNode       node;
   wizard::TextSource* source = nullptr;
   std::string         text;
   size_t              begin = 0;
   size_t              end   = 0;
   if (!ScenarioEdit::FindPlatform(platform, node, error))
   {
      result.mMessage = error;
      return result;
   }
   if (!ScenarioEdit::GetDefinition(node, source, text, begin, end, error))
   {
      result.mMessage = error;
      return result;
   }

   const std::vector<ScenarioEdit::TextLine> lines = ScenarioEdit::SplitLines(text, begin);
   size_t                                    endPlatform = std::string::npos;
   for (size_t i = 0; i < lines.size(); ++i)
   {
      if ((lines[i].mIndent == 0) && ScenarioEdit::LineHasKeyword(lines[i], "end_platform"))
      {
         endPlatform = i;
      }
   }
   if (endPlatform == std::string::npos)
   {
      result.mMessage = "Unable to find 'end_platform' in the definition of '" + platform + "'.";
      return result;
   }

   // 一个平台只能有一条航线，已存在时提示改用 add_waypoint 扩展
   for (size_t i = 0; i < lines.size(); ++i)
   {
      if ((lines[i].mIndent == indentUnit.size()) && ScenarioEdit::LineHasKeyword(lines[i], "route"))
      {
         result.mMessage = "Platform '" + platform + "' already has a route; use add_waypoint to extend it.";
         return result;
      }
   }

   std::string block = indentUnit + "route\n";
   // 首航路点缺省速度，避免 Wizard 无法计算航线
   std::string effectiveSpeed = speed;
   if (!latitude.empty() && effectiveSpeed.empty())
   {
      effectiveSpeed = "500 mph";
   }
   block += ScenarioEdit::BuildWaypointText(indentUnit + indentUnit,
                                            indentUnit,
                                            "",
                                            latitude,
                                            longitude,
                                            altitude,
                                            effectiveSpeed);
   block += indentUnit + "end_route\n";

   const ScenarioEdit::Edit edit{lines[endPlatform].mBegin, lines[endPlatform].mBegin - 1, block};
   if (!ScenarioEdit::ApplyEdits(source, {edit}, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Added a route to platform '" + platform + "'.\n\n" + block;
   return result;
}
} // namespace McpServer
