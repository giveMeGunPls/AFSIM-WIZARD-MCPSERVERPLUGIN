// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSetCurrentStartupFile.hpp"

#include <string>
#include <vector>

#include "McpFileToolSupport.hpp"
#include "McpStartupToolSupport.hpp"
#include "Project.hpp"

namespace McpServer
{
std::string SetCurrentStartupFileTool::Name() const
{
   return "set_current_startup_file";
}

std::string SetCurrentStartupFileTool::Title() const
{
   return "设置当前启动文件";
}

std::string SetCurrentStartupFileTool::Description() const
{
   return "把指定文件设为唯一的主启动文件，替换原启动文件列表，等效 Project Browser 的 "
          "\"Set as Startup File\"。路径须存在、为文本文件且位于项目目录内。"
          "默认同时保存项目文件；save=false 时仅修改内存。";
}

std::vector<ToolParameter> SetCurrentStartupFileTool::Parameters() const
{
   return {{"file", "要设为主启动文件的路径（绝对路径或相对于项目目录）", true},
           {"save", "可选：是否保存项目文件，缺省 true", false}};
}

ToolTextResult SetCurrentStartupFileTool::Execute(const ToolArguments& aArguments) const
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

   wizard::Project*          project = wizard::Project::Instance();
   const std::vector<UtPath> current = project->GetStartupFiles();
   const UtPath              newFile(absolutePath);

   if ((current.size() == 1) && StartupTools::StartupPathsEqual(current.front(), newFile))
   {
      result.mSuccess = true;
      result.mMessage = "The specified file is already the only startup file:\n" +
                        StartupTools::DescribeStartupFiles(current);
      return result;
   }

   project->SetStartupFiles(std::vector<UtPath>{newFile});

   std::string note;
   if (!StartupTools::SaveProjectIfRequested(save, note, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Startup file list replaced with 1 file." + note + "\n" +
                     StartupTools::DescribeStartupFiles(project->GetStartupFiles());
   return result;
}
} // namespace McpServer
