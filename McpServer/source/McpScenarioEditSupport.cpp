// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpScenarioEditSupport.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

#include "EditorPrefObject.hpp"
#include "FileTools/McpFileToolSupport.hpp"
#include "Project.hpp"
#include "ProjectWorkspace.hpp"
#include "TextSource.hpp"
#include "UtPath.hpp"
#include "UtTextDocument.hpp"
#include "WkfEnvironment.hpp"
#include "WsfPM_Root.hpp"
#include "WsfPProxy.hpp"
#include "WsfPProxyObjectMap.hpp"

namespace
{
//! 分支与空白判断：脚本以空格缩进，换行符不参与行内容
bool IsSpace(char aChar)
{
   return std::isspace(static_cast<unsigned char>(aChar)) != 0;
}

//! 取文档对应的 TextSource；项目内的脚本文件都由 ProjectWorkspace 管理
wizard::TextSource* SourceForDocument(UtTextDocument* aDocument)
{
   wizard::ProjectWorkspace* workspace = wizard::ProjectWorkspace::Instance();
   if (!aDocument || !workspace)
   {
      return nullptr;
   }
   return workspace->FindSource(aDocument->GetFilePath().GetSystemPath(), false, false);
}
} // namespace

namespace McpServer
{
namespace ScenarioEdit
{
std::vector<TextLine> SplitLines(const std::string& aText, size_t aBaseOffset)
{
   std::vector<TextLine> lines;

   size_t start = 0;
   while (true)
   {
      const size_t newline    = aText.find('\n', start);
      const size_t contentEnd = (newline == std::string::npos) ? aText.size() : newline;

      TextLine line;
      line.mBegin      = aBaseOffset + start;
      line.mText       = aText.substr(start, contentEnd - start);
      line.mHasNewline = (newline != std::string::npos);
      line.mEnd        = line.mText.empty() ? (line.mBegin - 1) : (line.mBegin + line.mText.size() - 1);
      line.mContentEnd = line.mEnd;

      line.mIndent = 0;
      while ((line.mIndent < line.mText.size()) && (line.mText[line.mIndent] == ' '))
      {
         ++line.mIndent;
      }

      lines.push_back(line);

      if (newline == std::string::npos)
      {
         break;
      }
      start = newline + 1;
   }
   return lines;
}

std::string Argument(const ToolArguments& aArguments, const std::string& aName)
{
   const ToolArguments::const_iterator iter = aArguments.find(aName);
   return (iter == aArguments.end()) ? std::string() : iter->second;
}

std::string IndentUnit()
{
   unsigned int              tabSpace = 3;
   wizard::EditorPrefObject* pref     = wkfEnv.GetPreferenceObject<wizard::EditorPrefObject>();
   if (pref)
   {
      tabSpace = pref->TabSpace();
   }
   return std::string(tabSpace, ' ');
}

std::vector<std::string> SplitTokens(const std::string& aText)
{
   std::vector<std::string> tokens;
   size_t                   pos = 0;
   while (pos < aText.size())
   {
      while ((pos < aText.size()) && IsSpace(aText[pos]))
      {
         ++pos;
      }
      const size_t begin = pos;
      while ((pos < aText.size()) && !IsSpace(aText[pos]))
      {
         ++pos;
      }
      if (pos > begin)
      {
         tokens.push_back(aText.substr(begin, pos - begin));
      }
   }
   return tokens;
}

bool ParseIndex(const std::string& aValue, size_t& aIndex)
{
   if (aValue.empty())
   {
      return false;
   }
   for (const char character : aValue)
   {
      if (std::isdigit(static_cast<unsigned char>(character)) == 0)
      {
         return false;
      }
   }
   try
   {
      aIndex = static_cast<size_t>(std::stoul(aValue));
   }
   catch (...)
   {
      return false;
   }
   return true;
}

bool RequireParsedProject(std::string& aError)
{
   wizard::Project* project = wizard::Project::Instance();
   if (!project || !project->Proxy())
   {
      aError = "No project is open in AFSIM Wizard, or the scenario has not been parsed yet.";
      return false;
   }
   return true;
}

bool FindPlatform(const std::string& aName, WsfPProxyNode& aNode, std::string& aError)
{
   if (!RequireParsedProject(aError))
   {
      return false;
   }

   aNode = WsfPM_Root(wizard::Project::Instance()->Proxy()).platforms().Find(aName);
   if (!aNode.IsValid())
   {
      aError = "Platform '" + aName + "' does not exist in this scenario.";
      return false;
   }
   return true;
}

bool GetDefinition(const WsfPProxyNode& aNode,
                   wizard::TextSource*& aSource,
                   std::string&         aText,
                   size_t&              aBegin,
                   size_t&              aEnd,
                   std::string&         aError)
{
   const UtTextDocumentRange range = aNode.GetSourceDocumentRange();
   if (!range.Valid() || (range.GetEnd() == ut::npos) || (range.GetEnd() < range.GetBegin()))
   {
      aError = "The definition has no source location in the scenario files.";
      return false;
   }

   aSource = SourceForDocument(range.mSource);
   if (!aSource)
   {
      aError = "Unable to locate the script file that contains this definition.";
      return false;
   }
   if (!FileTools::EnsureLoaded(aSource, aError))
   {
      return false;
   }

   aBegin = range.GetBegin();
   aEnd   = std::min(range.GetEnd(), TextEndIndex(aSource));
   if (aEnd < aBegin)
   {
      aError = "The definition lies outside the current script file contents.";
      return false;
   }

   aText.assign(aSource->GetPointer(aBegin, aEnd - aBegin + 1), aEnd - aBegin + 1);
   return true;
}

size_t TextEndIndex(wizard::TextSource* aSource)
{
   // 文档正文末尾带一个 '\0' 结束符（UtTextDocument::Read）
   if (!aSource || (aSource->Size() < 2))
   {
      return 0;
   }
   return aSource->Size() - 2;
}

std::string RelativePath(const std::string& aAbsolutePath)
{
   wizard::Project* project = wizard::Project::Instance();
   if (!project)
   {
      return aAbsolutePath;
   }
   return project->ProjectDirectory().GetRelativePath(UtPath(aAbsolutePath)).GetSystemPath();
}

bool ApplyEdits(wizard::TextSource* aSource, std::vector<Edit> aEdits, std::string& aError)
{
   if (!aSource)
   {
      aError = "Unable to edit the script file: no text source.";
      return false;
   }
   if (aEdits.empty())
   {
      return true;
   }
   if (!FileTools::EnsureLoaded(aSource, aError))
   {
      return false;
   }

   // 从文件尾部往前修改，前面的偏移才不会因为后面的编辑而失效
   std::stable_sort(aEdits.begin(),
                    aEdits.end(),
                    [](const Edit& aLhs, const Edit& aRhs) { return aLhs.mBegin > aRhs.mBegin; });

   // 同一位置的多个纯插入合并，保证插入顺序并避免重复改写同一字符
   std::vector<Edit> merged;
   for (const Edit& edit : aEdits)
   {
      const bool insert = (edit.mEnd == edit.mBegin - 1);
      if (insert && !merged.empty() && (merged.back().mEnd == merged.back().mBegin - 1) &&
          (merged.back().mBegin == edit.mBegin))
      {
         // 先出现的编辑位于更靠前的位置，因此后插入的文本要排在前面
         merged.back().mText.insert(0, edit.mText);
         continue;
      }
      merged.push_back(edit);
   }

   for (const Edit& edit : merged)
   {
      const bool insert = (edit.mEnd == edit.mBegin - 1);
      if (!insert)
      {
         aSource->Replace(UtTextRange(edit.mBegin, edit.mEnd), edit.mText);
         continue;
      }

      // UtTextRange(begin, begin-1) 会被归一化为长度 1，因此纯插入必须保留原有字符：
      // 以“插入文本 + 原字符”替换该字符，效果等同于插入。
      if ((edit.mBegin + 1) >= aSource->Size())
      {
         aSource->Append(edit.mText);
      }
      else
      {
         const char following = (*aSource)[edit.mBegin];
         aSource->Replace(UtTextRange(edit.mBegin, edit.mBegin), edit.mText + following);
      }
   }
   return true;
}

bool AppendText(wizard::TextSource* aSource, const std::string& aText, std::string& aError)
{
   if (!aSource)
   {
      aError = "Unable to edit the script file: no text source.";
      return false;
   }
   if (aText.empty())
   {
      return true;
   }
   if (!FileTools::EnsureLoaded(aSource, aError))
   {
      return false;
   }

   // TextSource::Append 会在末尾结束符之前插入内容
   aSource->Append(aText);
   return true;
}

bool LineHasKeyword(const TextLine& aLine, const std::string& aKeyword)
{
   if (aLine.mText.size() < aLine.mIndent)
   {
      return false;
   }
   const std::string rest = aLine.mText.substr(aLine.mIndent);
   if (rest.compare(0, aKeyword.size(), aKeyword) != 0)
   {
      return false;
   }
   return (rest.size() == aKeyword.size()) || IsSpace(rest[aKeyword.size()]);
}

size_t LineEndIncludingNewline(const TextLine& aLine)
{
   if (aLine.mHasNewline)
   {
      return (aLine.mText.empty() ? aLine.mBegin : (aLine.mBegin + aLine.mText.size()));
   }
   return aLine.mEnd;
}

std::string BuildWaypointText(const std::string& aWaypointIndent,
                              const std::string& aIndent,
                              const std::string& aLabel,
                              const std::string& aLatitude,
                              const std::string& aLongitude,
                              const std::string& aAltitude,
                              const std::string& aSpeed)
{
   std::string text;
   if (!aLabel.empty())
   {
      text += aWaypointIndent + "label " + aLabel + "\n";
   }

   if (!aLatitude.empty() && !aLongitude.empty())
   {
      text += aWaypointIndent + "position " + aLatitude + " " + aLongitude;
      if (!aAltitude.empty())
      {
         text += " altitude " + aAltitude;
      }
      text += "\n";
   }
   else if (!aAltitude.empty())
   {
      text += aWaypointIndent + "altitude " + aAltitude + "\n";
   }

   if (!aSpeed.empty())
   {
      text += aWaypointIndent + aIndent + "speed " + aSpeed + "\n";
   }
   return text;
}

bool LocateRouteBlock(const std::string& aPlatform,
                      const std::string& aScope,
                      const std::string& aRouteName,
                      RouteLocation&     aLocation,
                      std::string&       aError)
{
   aLocation = RouteLocation();

   if (aScope == "global")
   {
      if (aRouteName.empty())
      {
         aError = "A global route requires 'route' (its name).";
         return false;
      }
      if (!RequireParsedProject(aError))
      {
         return false;
      }

      const WsfPProxyNode node =
         WsfPM_Root(wizard::Project::Instance()->Proxy()).routes().Find(aRouteName);
      if (!node.IsValid())
      {
         aError = "Global route '" + aRouteName + "' does not exist in this scenario.";
         return false;
      }

      size_t begin = 0;
      size_t end   = 0;
      if (!GetDefinition(node, aLocation.mSource, aLocation.mText, begin, end, aError))
      {
         return false;
      }
      aLocation.mBegin      = begin;
      aLocation.mEnd        = end;
      aLocation.mStartIndent = 0;
      return true;
   }

   if (aPlatform.empty())
   {
      aError = "A platform route requires 'platform'.";
      return false;
   }

   WsfPProxyNode node;
   if (!FindPlatform(aPlatform, node, aError))
   {
      return false;
   }

   std::string text;
   size_t      begin = 0;
   size_t      end   = 0;
   if (!GetDefinition(node, aLocation.mSource, text, begin, end, aError))
   {
      return false;
   }

   const std::vector<TextLine> lines      = SplitLines(text, begin);
   const std::string           indentUnit = IndentUnit();

   size_t startIndex = std::string::npos;
   for (size_t i = 0; i < lines.size(); ++i)
   {
      if ((lines[i].mIndent != indentUnit.size()) || !LineHasKeyword(lines[i], "route"))
      {
         continue;
      }
      if (!aRouteName.empty())
      {
         const std::vector<std::string> tokens = SplitTokens(lines[i].mText);
         if ((tokens.size() < 2) || (tokens[1] != aRouteName))
         {
            continue;
         }
      }
      startIndex = i;
      break;
   }

   if (startIndex == std::string::npos)
   {
      aError = "Platform '" + aPlatform + "' has no matching route block.";
      return false;
   }

   size_t endIndex = std::string::npos;
   for (size_t i = startIndex + 1; i < lines.size(); ++i)
   {
      if ((lines[i].mIndent == lines[startIndex].mIndent) && LineHasKeyword(lines[i], "end_route"))
      {
         endIndex = i;
         break;
      }
   }
   if (endIndex == std::string::npos)
   {
      aError = "The route block is not terminated by 'end_route'.";
      return false;
   }

   aLocation.mBegin       = lines[startIndex].mBegin;
   aLocation.mEnd         = LineEndIncludingNewline(lines[endIndex]);
   aLocation.mStartIndent = lines[startIndex].mIndent;
   aLocation.mText.assign(aLocation.mSource->GetPointer(aLocation.mBegin,
                                                        aLocation.mEnd - aLocation.mBegin + 1),
                          aLocation.mEnd - aLocation.mBegin + 1);
   return true;
}

bool FindEndRouteLine(const std::vector<TextLine>& aLines, size_t aStartIndent, size_t& aIndex)
{
   for (size_t i = 0; i < aLines.size(); ++i)
   {
      if ((aLines[i].mIndent == aStartIndent) && LineHasKeyword(aLines[i], "end_route"))
      {
         aIndex = i;
         return true;
      }
   }
   return false;
}

std::vector<size_t> FindWaypointPositions(const std::vector<TextLine>& aLines, size_t aStartIndent)
{
   size_t waypointIndent = std::string::npos;
   for (size_t i = 0; i < aLines.size(); ++i)
   {
      if ((aLines[i].mIndent > aStartIndent) && LineHasKeyword(aLines[i], "position"))
      {
         waypointIndent = aLines[i].mIndent;
         break;
      }
   }

   std::vector<size_t> positions;
   if (waypointIndent == std::string::npos)
   {
      return positions;
   }

   for (size_t i = 0; i < aLines.size(); ++i)
   {
      if ((aLines[i].mIndent == waypointIndent) && LineHasKeyword(aLines[i], "position"))
      {
         positions.push_back(i);
      }
   }
   return positions;
}
} // namespace ScenarioEdit
} // namespace McpServer
