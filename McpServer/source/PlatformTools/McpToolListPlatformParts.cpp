// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolListPlatformParts.hpp"

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

//! mover/fuel 在脚本中没有名字，只有类型
bool IsUnnamedKind(const std::string& aKind)
{
   return (aKind == "mover") || (aKind == "fuel");
}
} // namespace

namespace McpServer
{
std::string ListPlatformPartsTool::Name() const
{
   return "list_platform_parts";
}

std::string ListPlatformPartsTool::Title() const
{
   return "列出平台部件";
}

std::string ListPlatformPartsTool::Description() const
{
   return "列出平台定义中声明的部件（传感器、武器、通信设备、处理器、运动模型、燃料），"
          "并给出其类别、名称、类型与行号。";
}

std::vector<ToolParameter> ListPlatformPartsTool::Parameters() const
{
   return {
      {"platform", "平台名称。", true},
   };
}

ToolTextResult ListPlatformPartsTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string platform = ScenarioEdit::Argument(aArguments, "platform");

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

   std::vector<std::string> rows;
   for (size_t i = 0; i < lines.size(); ++i)
   {
      if (lines[i].mIndent != indentWidth)
      {
         continue;
      }

      std::vector<std::string> tokens = ScenarioEdit::SplitTokens(lines[i].mText);
      // 部件既可以直接声明，也可以带 'add' 前缀
      if (!tokens.empty() && (tokens[0] == "add"))
      {
         tokens.erase(tokens.begin());
      }
      if (tokens.size() < 2)
      {
         continue;
      }

      const std::string& kind = tokens[0];
      if (PartKinds().find(kind) == PartKinds().end())
      {
         continue;
      }

      std::string name;
      std::string type;
      if (IsUnnamedKind(kind))
      {
         name = "-";
         type = tokens[1];
      }
      else
      {
         if (tokens.size() < 3)
         {
            continue;
         }
         name = tokens[1];
         type = tokens[2];
      }

      rows.push_back(kind + "\t" + name + "\t" + type + "\t" + std::to_string(i + 1));
   }

   if (rows.empty())
   {
      result.mSuccess = true;
      result.mMessage = "Platform '" + platform + "' declares no parts.";
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Platform: " + platform + "\nKind\tName\tType\tLine\n";
   for (const std::string& row : rows)
   {
      result.mMessage += row + "\n";
   }
   return result;
}
} // namespace McpServer
