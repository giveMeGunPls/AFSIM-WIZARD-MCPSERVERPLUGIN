// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolFindPlatform.hpp"

#include <string>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace McpServer
{
std::string FindPlatformTool::Name() const
{
   return "find_platform";
}

std::string FindPlatformTool::Title() const
{
   return "查找平台";
}

std::string FindPlatformTool::Description() const
{
   return "在想定脚本中定位平台：返回其所在文件、行范围、平台类型以及完整的定义文本。";
}

std::vector<ToolParameter> FindPlatformTool::Parameters() const
{
   return {
      {"name", "要定位的平台名称。", true},
   };
}

ToolTextResult FindPlatformTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string name = ScenarioEdit::Argument(aArguments, "name");

   std::string         error;
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

   // 定义首行的第二个词是平台类型
   std::string type;
   const std::vector<std::string> headerTokens =
      ScenarioEdit::SplitTokens(ScenarioEdit::SplitLines(text, 0).front().mText);
   if (headerTokens.size() >= 3)
   {
      type = headerTokens[2];
   }

   const size_t firstLine = source->PositionToLineNumber(begin) + 1;
   const size_t lastLine  = source->PositionToLineNumber(end) + 1;

   result.mSuccess = true;
   result.mMessage = "Platform: " + name + "\nType: " + (type.empty() ? "(unknown)" : type) + "\nFile: " +
                     ScenarioEdit::RelativePath(source->GetFilePath().GetSystemPath()) +
                     "\nLines: " + std::to_string(firstLine) + "-" + std::to_string(lastLine) +
                     "\n\n" + text;
   return result;
}
} // namespace McpServer
