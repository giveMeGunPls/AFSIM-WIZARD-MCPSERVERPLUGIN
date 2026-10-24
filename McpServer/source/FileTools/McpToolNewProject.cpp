// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolNewProject.hpp"

#include <string>

#include <QDir>
#include <QFileInfo>
#include <QString>

#include "McpFileToolSupport.hpp"
#include "Project.hpp"
#include "ProjectWorkspace.hpp"
#include "RunManager.hpp"
#include "UtPath.hpp"

namespace McpServer
{
std::string NewProjectTool::Name() const
{
   return "new_project";
}

std::string NewProjectTool::Title() const
{
   return "New Project";
}

std::string NewProjectTool::Description() const
{
   return "创建新的 AFSIM Wizard 项目文件（.afproj）。'file' 必须为绝对路径并以 .afproj 结尾。"
          "若已有项目打开且存在未保存修改，将拒绝执行，请先调用 save_all 或 close_project。";
}

std::vector<ToolParameter> NewProjectTool::Parameters() const
{
   return {{"file", "项目文件绝对路径（以 .afproj 结尾）", true}};
}

ToolTextResult NewProjectTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   wizard::ProjectWorkspace* workspace = wizard::ProjectWorkspace::Instance();
   if (workspace == nullptr)
   {
      result.mMessage = "AFSIM Wizard workspace is not available.";
      return result;
   }

   const std::string fileArgument = FileTools::Argument(aArguments, "file");
   if (fileArgument.empty())
   {
      result.mMessage = "A project file path is required.";
      return result;
   }

   const QString path = QString::fromStdString(fileArgument);
   if (!QFileInfo(path).isAbsolute())
   {
      result.mMessage = "Project file path must be absolute: " + fileArgument;
      return result;
   }
   const QString absolutePath = QDir::cleanPath(path);
   if (!absolutePath.endsWith(".afproj", Qt::CaseInsensitive))
   {
      result.mMessage = "Project file must use the .afproj extension: " + absolutePath.toStdString();
      return result;
   }

   const std::string projectPath = absolutePath.toStdString();
   if (FileTools::PathExists(projectPath))
   {
      result.mMessage = "File already exists: " + projectPath;
      return result;
   }

   if (!FileTools::EnsureParentDirectory(projectPath, error))
   {
      result.mMessage = error;
      return result;
   }

   // 关闭旧项目会丢弃未保存修改，因此先检查并拒绝，避免数据丢失
   if ((wizard::Project::Instance() != nullptr) && FileTools::HasUnsavedChanges())
   {
      result.mMessage = "The current project has unsaved changes. Call save_all or close_project before creating a new project.";
      return result;
   }

   // 与 File > New Project 一致：先关闭当前项目
   if ((wizard::Project::Instance() != nullptr) && !workspace->RequestClose(true))
   {
      result.mMessage = "Could not close the currently open project.";
      return result;
   }

   if (!wizRunMgr.SetWorkingDirectoryToProject(projectPath))
   {
      result.mMessage = "Could not set the working directory to the new project location: " + projectPath;
      return result;
   }

   const UtPath fileNamePath(projectPath);
   UtPath       directoryPath = fileNamePath;
   directoryPath.Up();

   wizard::Project* projectPtr = new wizard::Project(workspace);
   if (!projectPtr->SetProjectLocation(directoryPath, fileNamePath))
   {
      projectPtr->CloseProject(true);
      result.mMessage = "Could not create project at: " + projectPath;
      return result;
   }

   if (!projectPtr->SaveProject())
   {
      result.mMessage = "Created project in memory but could not write project file: " + projectPath;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Created project '" + projectPath + "'.";
   return result;
}
} // namespace McpServer
