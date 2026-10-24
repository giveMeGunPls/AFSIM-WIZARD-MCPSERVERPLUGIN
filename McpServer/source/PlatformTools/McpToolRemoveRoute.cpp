// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolRemoveRoute.hpp"

#include <string>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace McpServer
{
std::string RemoveRouteTool::Name() const
{
   return "remove_route";
}

std::string RemoveRouteTool::Title() const
{
   return "删除航线";
}

std::string RemoveRouteTool::Description() const
{
   return "删除整个航线块：平台航线（scope 'platform'）或具名的全局航线（scope 'global'）。";
}

std::vector<ToolParameter> RemoveRouteTool::Parameters() const
{
   return {
      {"platform", "拥有该航线的平台。scope 为 'platform' 时必填。", false},
      {"scope", "取值为 'platform'（默认）或 'global'。", false},
      {"route", "平台航线的航线名称，或全局航线名称。", false},
   };
}

ToolTextResult RemoveRouteTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string platform = ScenarioEdit::Argument(aArguments, "platform");
   const std::string scope    = ScenarioEdit::Argument(aArguments, "scope");
   const std::string route    = ScenarioEdit::Argument(aArguments, "route");

   std::string error;
   ScenarioEdit::RouteLocation location;
   if (!ScenarioEdit::LocateRouteBlock(platform, scope.empty() ? "platform" : scope, route, location, error))
   {
      result.mMessage = error;
      return result;
   }

   const ScenarioEdit::Edit edit{location.mBegin, location.mEnd, std::string()};
   if (!ScenarioEdit::ApplyEdits(location.mSource, {edit}, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = (scope == "global") ? ("Deleted global route '" + route + "'.") : ("Deleted a route from platform '" + platform + "'.");
   return result;
}
} // namespace McpServer
