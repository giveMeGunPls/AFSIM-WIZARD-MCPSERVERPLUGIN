// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSetStartupFiles.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "McpFileToolSupport.hpp"
#include "McpStartupToolSupport.hpp"
#include "Project.hpp"

namespace McpServer
{
namespace
{
//! 去掉首尾空白（含行尾的 '\r'）
std::string Trim(const std::string& aValue)
{
   const char* whitespace = " \t\r\n";
   const size_t begin     = aValue.find_first_not_of(whitespace);
   if (begin == std::string::npos)
   {
      return std::string();
   }
   const size_t end = aValue.find_last_not_of(whitespace);
   return aValue.substr(begin, end - begin + 1);
}
} // namespace

std::string SetStartupFilesTool::Name() const
{
   return "set_startup_files";
}

std::string SetStartupFilesTool::Title() const
{
   return "设置启动文件列表";
}

std::string SetStartupFilesTool::Description() const
{
   return "按给定顺序整体替换启动文件列表，第一项为主启动文件，等效 Project Browser 的 "
          "\"Configure Startup Files\" 对话框。'files' 用换行分隔，至少提供一项；"
          "每一项都须存在、为文本文件且位于项目目录内，且不允许重复。"
          "默认同时保存项目文件；save=false 时仅修改内存。";
}

std::vector<ToolParameter> SetStartupFilesTool::Parameters() const
{
   return {{"files", "启动文件路径列表，每行一个，顺序即执行顺序（第一项为主启动文件）", true},
           {"save", "可选：是否保存项目文件，缺省 true", false}};
}

ToolTextResult SetStartupFilesTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   bool save = true;
   if (!StartupTools::ParseSaveArgument(aArguments, save, error))
   {
      result.mMessage = error;
      return result;
   }

   std::vector<std::string> lines;
   std::istringstream       stream(FileTools::Argument(aArguments, "files"));
   std::string              line;
   while (std::getline(stream, line))
   {
      const std::string trimmed = Trim(line);
      if (!trimmed.empty())
      {
         lines.push_back(trimmed);
      }
   }

   if (lines.empty())
   {
      result.mMessage = "Provide at least one startup file path in 'files' (one per line).";
      return result;
   }

   std::vector<UtPath> files;
   for (const std::string& candidate : lines)
   {
      std::string absolutePath;
      if (!StartupTools::PrepareStartupFile(candidate, absolutePath, error))
      {
         result.mMessage = error;
         return result;
      }

      const UtPath filePath(absolutePath);
      for (const UtPath& existing : files)
      {
         if (StartupTools::StartupPathsEqual(existing, filePath))
         {
            result.mMessage = "Duplicate startup file in list: " + absolutePath;
            return result;
         }
      }
      files.push_back(filePath);
   }

   wizard::Project* project = wizard::Project::Instance();
   if (StartupTools::SameStartupFiles(files, project->GetStartupFiles()))
   {
      result.mSuccess = true;
      result.mMessage = "Startup file list is unchanged:\n" + StartupTools::DescribeStartupFiles(files);
      return result;
   }

   project->SetStartupFiles(files);

   std::string note;
   if (!StartupTools::SaveProjectIfRequested(save, note, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Startup file list replaced (" + std::to_string(files.size()) + " file(s))." + note + "\n" +
                     StartupTools::DescribeStartupFiles(project->GetStartupFiles());
   return result;
}
} // namespace McpServer
