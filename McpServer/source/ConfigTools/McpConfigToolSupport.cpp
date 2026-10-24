// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpConfigToolSupport.hpp"

#include <utility>

#include "FileTools/McpFileToolSupport.hpp"
#include "McpScenarioEditSupport.hpp"
#include "McpScenarioQuery.hpp"
#include "Project.hpp"
#include "ProjectWorkspace.hpp"
#include "TextSource.hpp"
#include "UtPath.hpp"

namespace
{
//! 读取脚本正文；TextSource 在内存中的换行已归一化为 '\n'
bool LoadBody(wizard::TextSource* aSource, std::string& aBody, std::string& aError)
{
   if (!McpServer::FileTools::EnsureLoaded(aSource, aError))
   {
      return false;
   }
   if (aSource->Size() <= 1)
   {
      aBody.clear();
      return true;
   }
   const size_t end = McpServer::ScenarioEdit::TextEndIndex(aSource);
   aBody.assign(aSource->GetPointer(0, end + 1), end + 1);
   return true;
}

//! 追加内容，与已有正文之间留一个空行
bool AppendWithSeparator(wizard::TextSource* aSource,
                         const std::string&  aBody,
                         const std::string&  aText,
                         std::string&        aError)
{
   std::string addition;
   if (!aBody.empty())
   {
      if (aBody.back() != '\n')
      {
         addition += '\n';
      }
      addition += '\n';
   }
   addition += aText;
   return McpServer::ScenarioEdit::AppendText(aSource, addition, aError);
}

//! 在已有块文本中更新参数行；缺失的参数行插入到结束标签之前，其余内容保持不变
std::string ApplyBlockOptions(const std::string&                     aBlockText,
                              const std::vector<McpServer::ConfigTools::BlockOption>& aOptions)
{
   std::vector<McpServer::ScenarioEdit::TextLine> lines =
      McpServer::ScenarioEdit::SplitLines(aBlockText, 0);
   const std::string indent          = McpServer::ScenarioEdit::IndentUnit();
   const bool        trailingNewline = !aBlockText.empty() && (aBlockText.back() == '\n');

   std::vector<std::string> content;
   content.reserve(lines.size() + aOptions.size());
   for (const McpServer::ScenarioEdit::TextLine& line : lines)
   {
      content.push_back(line.mText);
   }

   std::vector<std::string> inserts;
   for (const McpServer::ConfigTools::BlockOption& option : aOptions)
   {
      bool found = false;
      for (size_t i = 0; i < lines.size(); ++i)
      {
         if (McpServer::ScenarioEdit::LineHasKeyword(lines[i], option.mKeyword))
         {
            content[i] = indent + option.mText;
            found      = true;
            break;
         }
      }
      if (!found)
      {
         inserts.push_back(indent + option.mText);
      }
   }

   // 块的最后一行一定是结束标签，缺失的参数行插在它之前
   const size_t endIndex = content.empty() ? 0 : (content.size() - 1);

   std::string text;
   for (size_t i = 0; i < content.size(); ++i)
   {
      if (i == endIndex)
      {
         for (const std::string& insert : inserts)
         {
            text += insert + '\n';
         }
      }
      text += content[i];
      if (((i + 1) < content.size()) || trailingNewline)
      {
         text += '\n';
      }
   }
   return text;
}
} // namespace

namespace McpServer
{
namespace ConfigTools
{
bool ValidateValue(const std::string& aName, const std::string& aValue, std::string& aError)
{
   if (aValue.empty())
   {
      aError = "Missing '" + aName + "' argument.";
      return false;
   }
   if (aValue.find_first_of("\r\n") != std::string::npos)
   {
      aError = "Argument '" + aName + "' must be a single line.";
      return false;
   }
   return true;
}

bool ResolveTargetSource(const std::string&   aFileArgument,
                         wizard::TextSource*& aSource,
                         std::string&         aTargetRelative,
                         std::string&         aError)
{
   aSource = nullptr;

   wizard::Project* project = wizard::Project::Instance();
   if (!project)
   {
      aError = "No project is open in AFSIM Wizard.";
      return false;
   }

   std::string absoluteFile;
   if (!aFileArgument.empty())
   {
      absoluteFile = FileTools::ResolveProjectPath(aFileArgument, aError);
      if (absoluteFile.empty())
      {
         return false;
      }
   }
   else
   {
      const std::vector<UtPath>& startupFiles = project->GetStartupFiles();
      if (!startupFiles.empty())
      {
         absoluteFile = startupFiles.front().GetSystemPath();
      }
      else
      {
         // 没有启动文件时回退到平台定义最多的脚本，与场景编辑类工具保持一致
         const std::string fallback = ScenarioQuery::DefaultTargetFile();
         if (fallback.empty())
         {
            aError = "No target script file is available in this project; pass 'file' explicitly.";
            return false;
         }
         absoluteFile = FileTools::ResolveProjectPath(fallback, aError);
         if (absoluteFile.empty())
         {
            return false;
         }
      }
   }

   wizard::ProjectWorkspace* workspace = wizard::ProjectWorkspace::Instance();
   aSource                             = workspace ? workspace->FindSource(absoluteFile, false, false) : nullptr;
   if (!aSource)
   {
      aError = "Unable to find the target script file: " + absoluteFile;
      return false;
   }

   aTargetRelative = ScenarioEdit::RelativePath(absoluteFile);
   return true;
}

bool UpsertCommand(wizard::TextSource* aSource,
                   const std::string& aKeyword,
                   const std::string& aCommandText,
                   std::string&       aError)
{
   std::string body;
   if (!LoadBody(aSource, body, aError))
   {
      return false;
   }

   const std::vector<ScenarioEdit::TextLine> lines = ScenarioEdit::SplitLines(body, 0);
   for (const ScenarioEdit::TextLine& line : lines)
   {
      if ((line.mIndent == 0) && ScenarioEdit::LineHasKeyword(line, aKeyword))
      {
         ScenarioEdit::Edit edit;
         edit.mBegin = line.mBegin;
         edit.mEnd   = ScenarioEdit::LineEndIncludingNewline(line);
         edit.mText  = aCommandText;
         return ScenarioEdit::ApplyEdits(aSource, {edit}, aError);
      }
   }

   return AppendWithSeparator(aSource, body, aCommandText, aError);
}

bool UpsertBlock(wizard::TextSource*            aSource,
                 const std::string&              aStartKeyword,
                 const std::string&              aEndKeyword,
                 const std::string&              aBlockText,
                 const std::vector<BlockOption>& aOptions,
                 std::string&                    aError)
{
   std::string body;
   if (!LoadBody(aSource, body, aError))
   {
      return false;
   }

   const std::vector<ScenarioEdit::TextLine> lines = ScenarioEdit::SplitLines(body, 0);

   size_t startIndex = std::string::npos;
   for (size_t i = 0; i < lines.size(); ++i)
   {
      if ((lines[i].mIndent == 0) && ScenarioEdit::LineHasKeyword(lines[i], aStartKeyword))
      {
         startIndex = i;
         break;
      }
   }

   if (startIndex == std::string::npos)
   {
      return AppendWithSeparator(aSource, body, aBlockText, aError);
   }

   size_t endIndex = std::string::npos;
   for (size_t i = startIndex + 1; i < lines.size(); ++i)
   {
      if ((lines[i].mIndent == 0) && ScenarioEdit::LineHasKeyword(lines[i], aEndKeyword))
      {
         endIndex = i;
         break;
      }
   }
   if (endIndex == std::string::npos)
   {
      aError = "The existing '" + aStartKeyword + "' block is not terminated by '" + aEndKeyword + "'.";
      return false;
   }

   const size_t begin = lines[startIndex].mBegin;
   const size_t stop  = ScenarioEdit::LineEndIncludingNewline(lines[endIndex]);
   const std::string blockText = body.substr(begin, stop - begin + 1);

   ScenarioEdit::Edit edit;
   edit.mBegin = begin;
   edit.mEnd   = stop;
   edit.mText  = ApplyBlockOptions(blockText, aOptions);
   return ScenarioEdit::ApplyEdits(aSource, {edit}, aError);
}
} // namespace ConfigTools
} // namespace McpServer
