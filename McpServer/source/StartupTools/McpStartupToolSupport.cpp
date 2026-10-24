// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpStartupToolSupport.hpp"

#include <sstream>

#include <QString>

#include "McpFileToolSupport.hpp"
#include "Project.hpp"
#include "Util.hpp"

namespace McpServer
{
namespace StartupTools
{
bool PrepareStartupFile(const std::string& aPathArgument, std::string& aAbsolutePath, std::string& aError)
{
   if (!FileTools::RequireProject(aError))
   {
      return false;
   }

   aAbsolutePath = FileTools::ResolveProjectPath(aPathArgument, aError);
   if (aAbsolutePath.empty())
   {
      return false;
   }

   const UtPath filePath(aAbsolutePath);
   if (filePath.Stat() != UtPath::cFILE)
   {
      aError = "File does not exist: " + aAbsolutePath;
      return false;
   }

   // 与 Project Browser 的上下文菜单一致，只允许文本文件作为启动文件
   if (wizard::Util::IsNonTextFile(QString::fromStdString(aAbsolutePath)))
   {
      aError = "Only text files can be startup files: " + aAbsolutePath;
      return false;
   }
   return true;
}

bool StartupPathsEqual(const UtPath& aLhs, const UtPath& aRhs)
{
   return FileTools::PathsEqual(aLhs.GetSystemPath(), aRhs.GetSystemPath());
}

bool SameStartupFiles(const std::vector<UtPath>& aLhs, const std::vector<UtPath>& aRhs)
{
   if (aLhs.size() != aRhs.size())
   {
      return false;
   }
   for (size_t i = 0; i < aLhs.size(); ++i)
   {
      if (!StartupPathsEqual(aLhs[i], aRhs[i]))
      {
         return false;
      }
   }
   return true;
}

std::string DescribeStartupFiles(const std::vector<UtPath>& aFiles)
{
   wizard::Project* project = wizard::Project::Instance();
   if ((project == nullptr) || aFiles.empty())
   {
      return std::string();
   }

   UtPath projectDirectory = project->ProjectDirectory();
   std::ostringstream text;
   for (size_t i = 0; i < aFiles.size(); ++i)
   {
      const UtPath& file = aFiles[i];
      text << "  " << (i + 1) << ". ";
      if (i == 0)
      {
         text << "[primary] ";
      }
      text << projectDirectory.GetRelativePath(file).GetSystemPath() << "  (" << file.GetSystemPath() << ")";
      if (file.Stat() != UtPath::cFILE)
      {
         text << "  [missing]";
      }
      text << "\n";
   }
   return text.str();
}

bool ParseSaveArgument(const ToolArguments& aArguments, bool& aSave, std::string& aError)
{
   const std::string value = FileTools::Argument(aArguments, "save");
   if (value.empty())
   {
      aSave = true;
      return true;
   }
   return FileTools::ParseBoolean(value, aSave, aError);
}

bool SaveProjectIfRequested(bool aSave, std::string& aNote, std::string& aError)
{
   wizard::Project* project = wizard::Project::Instance();
   if (project == nullptr)
   {
      aError = "No project is open in AFSIM Wizard.";
      return false;
   }

   if (!aSave)
   {
      aNote = " Startup file changes are not persisted; call save_project to write the project file.";
      return true;
   }

   UtPath projectFile;
   if (!project->ProjectFilePath(projectFile) || projectFile.GetSystemPath().empty())
   {
      aNote = " The project has no project file yet, so the change is in memory only; "
              "call save_project with 'file' to persist it.";
      return true;
   }

   if (!project->SaveProject())
   {
      aError = "The startup file list was updated, but saving the project file failed: " +
               projectFile.GetSystemPath();
      return false;
   }

   aNote = " Saved project file '" + projectFile.GetSystemPath() + "'.";
   return true;
}
} // namespace StartupTools
} // namespace McpServer
