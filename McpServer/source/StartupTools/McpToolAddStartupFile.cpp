// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolAddStartupFile.hpp"

#include <string>
#include <vector>

#include "McpFileToolSupport.hpp"
#include "McpStartupToolSupport.hpp"
#include "Project.hpp"

namespace McpServer
{
std::string AddStartupFileTool::Name() const
{
   return "add_startup_file";
}

std::string AddStartupFileTool::Title() const
{
   return "追加启动文件";
}

std::string AddStartupFileTool::Description() const
{
   return "把指定文件追加到启动文件列表末尾，等效 Project Browser 的 \"Add to Startup Files\"。"
          "文件已在列表中时不做改动。路径须存在、为文本文件且位于项目目录内。"
          "默认同时保存项目文件；save=false 时仅修改内存。";
}

std::vector<ToolParameter> AddStartupFileTool::Parameters() const
{
   return {{"file", "要追加的启动文件路径（绝对路径或相对于项目目录）", true},
           {"save", "可选：是否保存项目文件，缺省 true", false}};
}

ToolTextResult AddStartupFileTool::Execute(const ToolArguments& aArguments) const
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
   std::vector<UtPath>       files   = project->GetStartupFiles();
   const UtPath              newFile(absolutePath);

   for (size_t i = 0; i < files.size(); ++i)
   {
      if (StartupTools::StartupPathsEqual(files[i], newFile))
      {
         result.mSuccess = true;
         result.mMessage = "The file is already startup file " + std::to_string(i + 1) + ":\n" +
                           StartupTools::DescribeStartupFiles(files);
         return result;
      }
   }

   files.push_back(newFile);
   project->SetStartupFiles(files);

   std::string note;
   if (!StartupTools::SaveProjectIfRequested(save, note, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Added startup file at position " + std::to_string(files.size()) + "." + note + "\n" +
                     StartupTools::DescribeStartupFiles(project->GetStartupFiles());
   return result;
}
} // namespace McpServer
