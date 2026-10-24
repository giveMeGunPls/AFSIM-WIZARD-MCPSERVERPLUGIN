// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolRemoveZone.hpp"

#include <string>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "Project.hpp"
#include "TextSource.hpp"
#include "WsfPM_Root.hpp"
#include "WsfPProxy.hpp"
#include "WsfPProxyObjectMap.hpp"

namespace McpServer
{
std::string RemoveZoneTool::Name() const
{
   return "remove_zone";
}

std::string RemoveZoneTool::Title() const
{
   return "删除区域";
}

std::string RemoveZoneTool::Description() const
{
   return "从想定脚本中删除区域定义。";
}

std::vector<ToolParameter> RemoveZoneTool::Parameters() const
{
   return {
      {"name", "要删除的区域名称。", true},
   };
}

ToolTextResult RemoveZoneTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string name = ScenarioEdit::Argument(aArguments, "name");

   std::string error;
   if (!ScenarioEdit::RequireParsedProject(error))
   {
      result.mMessage = error;
      return result;
   }

   const WsfPProxyNode node = WsfPM_Root(wizard::Project::Instance()->Proxy()).zones().Find(name);
   if (!node.IsValid())
   {
      result.mMessage = "Zone '" + name + "' does not exist in this scenario.";
      return result;
   }

   wizard::TextSource* source = nullptr;
   std::string         text;
   size_t              begin = 0;
   size_t              end   = 0;
   if (!ScenarioEdit::GetDefinition(node, source, text, begin, end, error))
   {
      result.mMessage = error;
      return result;
   }

   // 连同紧随其后的换行一起删除，避免留下空行
   const size_t lastChar = ScenarioEdit::TextEndIndex(source);
   if ((end < lastChar) && ((*source)[end + 1] == '\n'))
   {
      ++end;
   }

   const ScenarioEdit::Edit edit{begin, end, std::string()};
   if (!ScenarioEdit::ApplyEdits(source, {edit}, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Deleted zone '" + name + "'.";
   return result;
}
} // namespace McpServer
