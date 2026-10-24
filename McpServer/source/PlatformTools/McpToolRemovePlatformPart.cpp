// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolRemovePlatformPart.hpp"

#include <set>
#include <string>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace
{
const std::set<std::string>& PartKinds()
{
   static const std::set<std::string> kinds = {"sensor", "weapon", "comm", "processor", "mover", "fuel"};
   return kinds;
}
} // namespace

namespace McpServer
{
std::string RemovePlatformPartTool::Name() const
{
   return "remove_platform_part";
}

std::string RemovePlatformPartTool::Title() const
{
   return "删除平台部件";
}

std::string RemovePlatformPartTool::Description() const
{
   return "从平台定义中删除一个部件块。通过 'name' 指定部件，"
          "或对没有名称的运动模型与燃料部件使用 'kind' 指定。";
}

std::vector<ToolParameter> RemovePlatformPartTool::Parameters() const
{
   return {
      {"platform", "平台名称。", true},
      {"name", "要删除的部件名称（传感器/武器/通信设备/处理器）。", false},
      {"kind", "部件没有名称时用于删除的部件类别（运动模型/燃料）。", false},
   };
}

ToolTextResult RemovePlatformPartTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string platform = ScenarioEdit::Argument(aArguments, "platform");
   const std::string name     = ScenarioEdit::Argument(aArguments, "name");
   const std::string kindOnly = ScenarioEdit::Argument(aArguments, "kind");

   if (name.empty() && kindOnly.empty())
   {
      result.mMessage = "Provide 'name', or 'kind' for mover/fuel parts.";
      return result;
   }
   if (!kindOnly.empty() && (PartKinds().find(kindOnly) == PartKinds().end()))
   {
      result.mMessage = "Unknown part kind '" + kindOnly + "'.";
      return result;
   }

   std::string         error;
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

   const std::vector<ScenarioEdit::TextLine> lines      = ScenarioEdit::SplitLines(text, begin);
   const size_t                              indentWidth = ScenarioEdit::IndentUnit().size();

   size_t            startIndex = std::string::npos;
   std::string       foundKind;
   for (size_t i = 0; i < lines.size(); ++i)
   {
      if (lines[i].mIndent != indentWidth)
      {
         continue;
      }

      std::vector<std::string> tokens = ScenarioEdit::SplitTokens(lines[i].mText);
      if (!tokens.empty() && (tokens[0] == "add"))
      {
         tokens.erase(tokens.begin());
      }
      if (tokens.size() < 2)
      {
         continue;
      }

      const std::string& kind = tokens[0];
      if ((PartKinds().find(kind) == PartKinds().end()) || (!kindOnly.empty() && (kind != kindOnly)))
      {
         continue;
      }

      if (!name.empty())
      {
         const bool unnamed = (kind == "mover") || (kind == "fuel");
         if (unnamed || (tokens.size() < 3) || (tokens[1] != name))
         {
            continue;
         }
      }

      startIndex = i;
      foundKind  = kind;
      break;
   }

   if (startIndex == std::string::npos)
   {
      result.mMessage = "No matching part was found in platform '" + platform + "'.";
      return result;
   }

   size_t endIndex = std::string::npos;
   for (size_t i = startIndex + 1; i < lines.size(); ++i)
   {
      if ((lines[i].mIndent == indentWidth) && ScenarioEdit::LineHasKeyword(lines[i], "end_" + foundKind))
      {
         endIndex = i;
         break;
      }
   }
   if (endIndex == std::string::npos)
   {
      result.mMessage = "The part block is not terminated by 'end_" + foundKind + "'.";
      return result;
   }

   const ScenarioEdit::Edit edit{lines[startIndex].mBegin,
                                 ScenarioEdit::LineEndIncludingNewline(lines[endIndex]),
                                 std::string()};
   if (!ScenarioEdit::ApplyEdits(source, {edit}, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Removed " + foundKind + (name.empty() ? "" : " '" + name + "'") + " from platform '" +
                     platform + "'.";
   return result;
}
} // namespace McpServer
