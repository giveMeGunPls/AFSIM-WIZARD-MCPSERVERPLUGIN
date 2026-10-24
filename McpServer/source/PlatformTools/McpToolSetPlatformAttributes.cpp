// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSetPlatformAttributes.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace McpServer
{
std::string SetPlatformAttributesTool::Name() const
{
   return "set_platform_attributes";
}

std::string SetPlatformAttributesTool::Title() const
{
   return "设置平台属性";
}

std::string SetPlatformAttributesTool::Description() const
{
   return "修改已有平台定义的顶层属性：阵营、图标、位置、高度、航向、俯仰角与滚转角。"
          "只重写你提供的属性；缺失的属性会按 AFSIM 脚本顺序插入到 'platform' 行之后。";
}

std::vector<ToolParameter> SetPlatformAttributesTool::Parameters() const
{
   return {
      {"name", "要修改的平台名称。", true},
      {"side", "阵营名称，如 'blue'。", false},
      {"icon", "图标名称，如 'F-16'。", false},
      {"latitude", "AFSIM 格式的纬度，如 '27:52:46.72'。须同时提供经度。", false},
      {"longitude", "AFSIM 格式的经度，如 '118:01:01.03'。须同时提供纬度。", false},
      {"altitude", "带单位的高度，如 '10000 m'。", false},
      {"heading", "带单位的航向，如 '90 deg'。", false},
      {"pitch", "带单位的俯仰角，如 '0 deg'。", false},
      {"roll", "带单位的滚转角，如 '0 deg'。", false},
   };
}

bool SetPlatformAttributesTool::IsSingleLine(const std::string& aValue)
{
   return (aValue.find('\n') == std::string::npos) && (aValue.find('\r') == std::string::npos);
}

bool SetPlatformAttributesTool::ParseRequest(const ToolArguments&    aArguments,
                                             std::string&            aName,
                                             std::vector<Attribute>& aAttributes,
                                             std::string&            aError)
{
   aName = ScenarioEdit::Argument(aArguments, "name");

   const std::string side      = ScenarioEdit::Argument(aArguments, "side");
   const std::string icon      = ScenarioEdit::Argument(aArguments, "icon");
   const std::string latitude  = ScenarioEdit::Argument(aArguments, "latitude");
   const std::string longitude = ScenarioEdit::Argument(aArguments, "longitude");
   const std::string altitude  = ScenarioEdit::Argument(aArguments, "altitude");
   const std::string heading   = ScenarioEdit::Argument(aArguments, "heading");
   const std::string pitch     = ScenarioEdit::Argument(aArguments, "pitch");
   const std::string roll      = ScenarioEdit::Argument(aArguments, "roll");

   if (latitude.empty() != longitude.empty())
   {
      aError = "Provide both 'latitude' and 'longitude', or neither.";
      return false;
   }

   // 固定顺序：side、icon、position，随后是姿态与高度
   if (!side.empty())
   {
      aAttributes.push_back({"side", side});
   }
   if (!icon.empty())
   {
      aAttributes.push_back({"icon", icon});
   }
   if (!latitude.empty())
   {
      aAttributes.push_back({"position", latitude + " " + longitude});
   }
   if (!altitude.empty())
   {
      aAttributes.push_back({"altitude", altitude});
   }
   if (!heading.empty())
   {
      aAttributes.push_back({"heading", heading});
   }
   if (!pitch.empty())
   {
      aAttributes.push_back({"pitch", pitch});
   }
   if (!roll.empty())
   {
      aAttributes.push_back({"roll", roll});
   }

   if (aAttributes.empty())
   {
      aError = "Provide at least one attribute to change.";
      return false;
   }

   for (const Attribute& attribute : aAttributes)
   {
      if (!IsSingleLine(attribute.mValue))
      {
         aError = "Attribute '" + attribute.mKeyword + "' must be a single line.";
         return false;
      }
   }
   return true;
}

ToolTextResult SetPlatformAttributesTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult          result;
   std::string             name;
   std::vector<Attribute>  attributes;
   std::string             error;
   if (!ParseRequest(aArguments, name, attributes, error))
   {
      result.mMessage = error;
      return result;
   }

   WsfPProxyNode       node;
   wizard::TextSource* source = nullptr;
   std::string         text;
   size_t              begin = 0;
   size_t              end   = 0;
   if (!ScenarioEdit::FindPlatform(name, node, error))
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
   const std::string                         indentUnit = ScenarioEdit::IndentUnit();

   // 定位 'platform <name> <type>' 行
   size_t headerIndex = std::string::npos;
   for (size_t i = 0; i < lines.size(); ++i)
   {
      if ((lines[i].mIndent == 0) && ScenarioEdit::LineHasKeyword(lines[i], "platform"))
      {
         headerIndex = i;
         break;
      }
   }
   if (headerIndex == std::string::npos)
   {
      result.mMessage = "Unable to locate the 'platform' line in the definition of '" + name + "'.";
      return result;
   }

   std::vector<ScenarioEdit::Edit> edits;
   std::vector<std::string>        insertedLines;
   std::vector<std::string>        changed;

   for (const Attribute& attribute : attributes)
   {
      size_t found = std::string::npos;
      for (size_t i = 0; i < lines.size(); ++i)
      {
         if ((i != headerIndex) && (lines[i].mIndent == indentUnit.size()) &&
             ScenarioEdit::LineHasKeyword(lines[i], attribute.mKeyword))
         {
            found = i;
            break;
         }
      }

      const std::string newContent = attribute.mKeyword + " " + attribute.mValue;
      if (found != std::string::npos)
      {
         const size_t contentBegin = lines[found].mBegin + lines[found].mIndent;
         const size_t contentEnd   = lines[found].mContentEnd;
         if (contentEnd < contentBegin)
         {
            edits.push_back({contentBegin, contentBegin - 1, newContent});
         }
         else
         {
            edits.push_back({contentBegin, contentEnd, newContent});
         }
      }
      else
      {
         insertedLines.push_back(indentUnit + newContent);
      }
      changed.push_back(attribute.mKeyword);
   }

   if (!insertedLines.empty())
   {
      std::string block;
      for (const std::string& line : insertedLines)
      {
         block += line + "\n";
      }

      size_t position = lines[headerIndex].mBegin + lines[headerIndex].mText.size();
      if (lines[headerIndex].mHasNewline)
      {
         ++position; // 插到换行符之后
      }
      else
      {
         block.insert(0, "\n"); // 头行在文件末尾，先补一个换行
      }
      edits.push_back({position, position - 1, block});
   }

   if (!ScenarioEdit::ApplyEdits(source, edits, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Updated platform '" + name + "': ";
   for (size_t i = 0; i < changed.size(); ++i)
   {
      result.mMessage += (i == 0 ? "" : ", ") + changed[i];
   }
   result.mMessage += ".";
   return result;
}
} // namespace McpServer
