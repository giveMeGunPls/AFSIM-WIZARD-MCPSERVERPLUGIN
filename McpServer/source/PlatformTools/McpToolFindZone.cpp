// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolFindZone.hpp"

#include <set>
#include <string>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "Project.hpp"
#include "TextSource.hpp"
#include "WsfPM_Root.hpp"
#include "WsfPProxy.hpp"
#include "WsfPProxyObjectMap.hpp"

namespace
{
//! 从定义文本中识别区域形状
std::string DetectShape(const std::string& aText)
{
   static const std::set<std::string> shapes = {"circular", "spherical", "elliptical", "polygonal"};
   const std::vector<McpServer::ScenarioEdit::TextLine> lines = McpServer::ScenarioEdit::SplitLines(aText, 0);
   for (const McpServer::ScenarioEdit::TextLine& line : lines)
   {
      const std::vector<std::string> tokens = McpServer::ScenarioEdit::SplitTokens(line.mText);
      if (tokens.size() == 1 && shapes.find(tokens[0]) != shapes.end())
      {
         return tokens[0];
      }
   }
   return std::string();
}
} // namespace

namespace McpServer
{
std::string FindZoneTool::Name() const
{
   return "find_zone";
}

std::string FindZoneTool::Title() const
{
   return "查找区域";
}

std::string FindZoneTool::Description() const
{
   return "在想定脚本中定位区域定义：返回其所在文件、行范围、形状以及完整的定义文本。";
}

std::vector<ToolParameter> FindZoneTool::Parameters() const
{
   return {
      {"name", "要定位的区域名称。", true},
   };
}

ToolTextResult FindZoneTool::Execute(const ToolArguments& aArguments) const
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

   const std::string shape     = DetectShape(text);
   const size_t      firstLine = source->PositionToLineNumber(begin) + 1;
   const size_t      lastLine  = source->PositionToLineNumber(end) + 1;

   result.mSuccess = true;
   result.mMessage = "Zone: " + name + "\nShape: " + (shape.empty() ? "(unknown)" : shape) + "\nFile: " +
                     ScenarioEdit::RelativePath(source->GetFilePath().GetSystemPath()) +
                     "\nLines: " + std::to_string(firstLine) + "-" + std::to_string(lastLine) + "\n\n" + text;
   return result;
}
} // namespace McpServer
