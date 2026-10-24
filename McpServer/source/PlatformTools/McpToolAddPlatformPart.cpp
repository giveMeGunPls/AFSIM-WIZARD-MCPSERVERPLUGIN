// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolAddPlatformPart.hpp"

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
std::string AddPlatformPartTool::Name() const
{
   return "add_platform_part";
}

std::string AddPlatformPartTool::Title() const
{
   return "添加平台部件";
}

std::string AddPlatformPartTool::Description() const
{
   return "向已有平台定义中插入部件块（传感器、武器、通信设备、处理器、运动模型或燃料）。"
          "有名字的部件须提供唯一的部件名称；运动模型与燃料只需提供类型。";
}

std::vector<ToolParameter> AddPlatformPartTool::Parameters() const
{
   return {
      {"platform", "接收该部件的平台名称。", true},
      {"kind", "部件类别：传感器、武器、通信设备、处理器、运动模型或燃料。", true},
      {"type", "部件类型，如 'WSF_RADAR_SENSOR'。", true},
      {"name", "唯一的部件名称。传感器/武器/通信设备/处理器必填；运动模型/燃料忽略此项。", false},
   };
}

ToolTextResult AddPlatformPartTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string platform = ScenarioEdit::Argument(aArguments, "platform");
   const std::string kind     = ScenarioEdit::Argument(aArguments, "kind");
   const std::string type     = ScenarioEdit::Argument(aArguments, "type");
   std::string       name     = ScenarioEdit::Argument(aArguments, "name");

   if (PartKinds().find(kind) == PartKinds().end())
   {
      result.mMessage = "Unknown part kind '" + kind +
                        "'. Use sensor, weapon, comm, processor, mover, or fuel.";
      return result;
   }

   const bool unnamed = IsUnnamedKind(kind);
   if (!unnamed && name.empty())
   {
      result.mMessage = "A name is required for part kind '" + kind + "'.";
      return result;
   }
   if (unnamed)
   {
      name.clear();
   }
   if ((name.find_first_of(" \t\r\n/#") != std::string::npos) || (type.find_first_of(" \t\r\n/#") != std::string::npos))
   {
      result.mMessage = "Part name and type must not contain spaces, '/', or '#'.";
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

   const std::vector<ScenarioEdit::TextLine> lines       = ScenarioEdit::SplitLines(text, begin);
   const std::string                         indentUnit  = ScenarioEdit::IndentUnit();
   size_t                                    endPlatform = std::string::npos;
   bool                                      duplicate   = false;
   for (size_t i = 0; i < lines.size(); ++i)
   {
      if ((lines[i].mIndent == 0) && ScenarioEdit::LineHasKeyword(lines[i], "end_platform"))
      {
         endPlatform = i;
      }
      if (lines[i].mIndent != indentUnit.size())
      {
         continue;
      }

      // 部件既可以直接声明，也可以带 'add' 前缀
      std::vector<std::string> tokens = ScenarioEdit::SplitTokens(lines[i].mText);
      if (!tokens.empty() && (tokens[0] == "add"))
      {
         tokens.erase(tokens.begin());
      }
      if (tokens.empty() || (tokens[0] != kind))
      {
         continue;
      }
      if (unnamed || ((tokens.size() >= 2) && (tokens[1] == name)))
      {
         duplicate = true;
      }
   }
   if (endPlatform == std::string::npos)
   {
      result.mMessage = "Unable to find 'end_platform' in the definition of '" + platform + "'.";
      return result;
   }
   if (duplicate)
   {
      result.mMessage = "Platform '" + platform + "' already has " +
                        (unnamed ? ("a '" + kind + "' part.") : ("a part named '" + name + "'."));
      return result;
   }

   std::string block = indentUnit + "add " + kind;
   if (!name.empty())
   {
      block += " " + name;
   }
   block += " " + type + "\n" + indentUnit + "end_" + kind + "\n";

   const ScenarioEdit::Edit edit{lines[endPlatform].mBegin, lines[endPlatform].mBegin - 1, block};
   if (!ScenarioEdit::ApplyEdits(source, {edit}, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Added " + kind + (name.empty() ? "" : " '" + name + "'") + " (" + type +
                     ") to platform '" + platform + "'.\n\n" + block;
   return result;
}
} // namespace McpServer
