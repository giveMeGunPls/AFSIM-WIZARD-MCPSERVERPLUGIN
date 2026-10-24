// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolRemoveStartupFile.hpp"

#include <string>
#include <vector>

#include "McpFileToolSupport.hpp"
#include "McpStartupToolSupport.hpp"
#include "Project.hpp"

namespace McpServer
{
std::string RemoveStartupFileTool::Name() const
{
   return "remove_startup_file";
}

std::string RemoveStartupFileTool::Title() const
{
   return "移除启动文件";
}

std::string RemoveStartupFileTool::Description() const
{
   return "从启动文件列表中移除指定文件，等效 Project Browser 的 \"Remove from Startup Files\"。"
          "不允许移除唯一的启动文件；如需替换请使用 set_current_startup_file。"
          "默认同时保存项目文件；save=false 时仅修改内存。";
}

std::vector<ToolParameter> RemoveStartupFileTool::Parameters() const
{
   return {{"file", "要移除的启动文件路径（绝对路径或相对于项目目录）", true},
           {"save", "可选：是否保存项目文件，缺省 true", false}};
}

ToolTextResult RemoveStartupFileTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   std::string absolutePath;
   if (!StartupTools::PrepareStartupFile(FileTools::Argument(aArguments, "file"), absolutePath, error))
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

   wizard::Project*    project = wizard::Project::Instance();
   std::vector<UtPath> files   = project->GetStartupFiles();
   const UtPath        target(absolutePath);

   size_t index = files.size();
   for (size_t i = 0; i < files.size(); ++i)
   {
      if (StartupTools::StartupPathsEqual(files[i], target))
      {
         index = i;
         break;
      }
   }

   if (index == files.size())
   {
      result.mMessage = "Not a startup file: " + absolutePath;
      return result;
   }

   if (files.size() == 1)
   {
      result.mMessage = "Cannot remove the only startup file. Use set_current_startup_file to replace it, "
                        "or set_startup_files to provide a new list.";
      return result;
   }

   files.erase(files.begin() + static_cast<std::ptrdiff_t>(index));
   project->SetStartupFiles(files);

   std::string note;
   if (!StartupTools::SaveProjectIfRequested(save, note, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Removed startup file at position " + std::to_string(index + 1) + "." + note + "\n" +
                     StartupTools::DescribeStartupFiles(project->GetStartupFiles());
   return result;
}
} // namespace McpServer
