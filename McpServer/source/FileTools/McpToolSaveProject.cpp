// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSaveProject.hpp"

#include <string>

#include <QString>

#include "McpFileToolSupport.hpp"
#include "Project.hpp"
#include "UtPath.hpp"

namespace McpServer
{
std::string SaveProjectTool::Name() const
{
   return "save_project";
}

std::string SaveProjectTool::Title() const
{
   return "Save Project";
}

std::string SaveProjectTool::Description() const
{
   return "保存当前项目的项目文件（.afproj）。项目尚未绑定项目文件时须提供 'file'；"
          "已绑定时 'file' 可省略，若提供则必须与当前项目文件一致。";
}

std::vector<ToolParameter> SaveProjectTool::Parameters() const
{
   return {{"file", "可选：项目文件路径；项目尚未绑定项目文件时为必填", false}};
}

ToolTextResult SaveProjectTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   wizard::Project* project = wizard::Project::Instance();

   UtPath      currentPath;
   const bool  hasProjectFile = project->ProjectFilePath(currentPath);
   std::string fileArgument   = FileTools::Argument(aArguments, "file");

   if (!hasProjectFile || currentPath.GetSystemPath().empty())
   {
      if (fileArgument.empty())
      {
         result.mMessage = "The project has no project file yet. Pass the 'file' argument (e.g. '/path/to/project.afproj').";
         return result;
      }

      const std::string absolutePath = FileTools::ResolveProjectPath(fileArgument, error);
      if (absolutePath.empty())
      {
         result.mMessage = error;
         return result;
      }
      if (!QString::fromStdString(absolutePath).endsWith(".afproj", Qt::CaseInsensitive))
      {
         result.mMessage = "Project file must use the .afproj extension: " + absolutePath;
         return result;
      }
      if (FileTools::PathExists(absolutePath))
      {
         result.mMessage = "File already exists: " + absolutePath;
         return result;
      }
      if (!FileTools::EnsureParentDirectory(absolutePath, error))
      {
         result.mMessage = error;
         return result;
      }
      if (!project->SetProjectFilePath(UtPath(absolutePath)))
      {
         result.mMessage = "Could not bind the project file: " + absolutePath;
         return result;
      }
   }
   else if (!fileArgument.empty())
   {
      const std::string absolutePath = FileTools::ResolveProjectPath(fileArgument, error);
      if (absolutePath.empty())
      {
         result.mMessage = error;
         return result;
      }
      if (!FileTools::PathsEqual(absolutePath, currentPath.GetSystemPath()))
      {
         result.mMessage = "The project is already bound to '" + currentPath.GetSystemPath() +
                           "'. Close and reopen the project to change its project file.";
         return result;
      }
   }

   if (!project->SaveProject())
   {
      result.mMessage = "Could not save project file. Verify the file is writable.";
      return result;
   }

   project->ProjectFilePath(currentPath);
   result.mSuccess = true;
   result.mMessage = "Saved project '" + currentPath.GetSystemPath() + "'.";
   return result;
}
} // namespace McpServer
