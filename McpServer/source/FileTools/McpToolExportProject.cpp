// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolExportProject.hpp"

#include <fstream>
#include <string>
#include <vector>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include "ExportFiles.hpp"
#include "McpFileToolSupport.hpp"
#include "ParseResults.hpp"
#include "Project.hpp"
#include "UtPath.hpp"
#include "UtTextDocument.hpp"

namespace McpServer
{
namespace
{
struct ExportEntry
{
   std::string     mSourcePath;
   std::string     mDestinationPath;
   UtTextDocument* mDocumentPtr = nullptr;
};
} // namespace

std::string ExportProjectTool::Name() const
{
   return "export_project";
}

std::string ExportProjectTool::Title() const
{
   return "Export Project";
}

std::string ExportProjectTool::Description() const
{
   return "把项目当前包括的全部文件（含未保存的编辑内容）导出到目标目录，等效于 File > Export Project。"
          "'directory' 需为已存在的目录，可写绝对路径或相对于项目目录的路径，但必须位于项目目录内。"
          "目标文件已存在时须显式设置 'overwrite' 为 true。";
}

std::vector<ToolParameter> ExportProjectTool::Parameters() const
{
   return {{"directory", "导出目标目录（必须已存在）", true},
           {"overwrite", "目标文件已存在时是否覆盖，默认 false", false}};
}

ToolTextResult ExportProjectTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   const std::string directoryPath =
      FileTools::ResolveProjectPath(FileTools::Argument(aArguments, "directory"), error);
   if (directoryPath.empty())
   {
      result.mMessage = error;
      return result;
   }
   if (UtPath(directoryPath).Stat() != UtPath::cDIRECTORY)
   {
      result.mMessage = "Export directory does not exist: " + directoryPath;
      return result;
   }

   bool overwrite = false;
   if (!FileTools::ParseBoolean(FileTools::Argument(aArguments, "overwrite"), overwrite, error))
   {
      result.mMessage = error;
      return result;
   }

   wizard::Project*      project      = wizard::Project::Instance();
   // 解析结果由独立解析线程异步产出，未就绪时直接报错，避免在 GUI 线程中等待
   if (!project->ParseIsUpToDate())
   {
      result.mMessage = "The project has not been parsed yet. Wait for parsing to complete and try again.";
      return result;
   }
   wizard::ParseResults* parseResults = project->WaitForParseResults();
   if (parseResults == nullptr)
   {
      result.mMessage = "The project has not been parsed yet. Wait for parsing to complete and try again.";
      return result;
   }

   // 复用现有导出路径规则，保持与 File > Export Project 相同的目录结构
   wizard::ExportFiles exporter(project->ProjectDirectory());
   const QString       exportDirectory = QString::fromStdString(directoryPath);

   std::vector<ExportEntry> entries;
   const UtPath&            projectDirectory = project->ProjectDirectory();
   for (const auto& include : parseResults->mFirstIncludes)
   {
      if (include.first == nullptr)
      {
         continue;
      }
      ExportEntry entry;
      entry.mSourcePath = include.first->GetFilePath().GetSystemPath();
      if (entry.mSourcePath.empty())
      {
         continue;
      }

      // 与 ExportFiles 一致：项目目录内的文件保留相对目录结构，
      // 项目外的文件放入 _external_items 下并按绝对路径结构保存，避免越出导出目录
      const UtPath sourcePath(entry.mSourcePath);
      const QString exportSourcePath = projectDirectory.IsAncestorOf(sourcePath)
                                          ? QString::fromStdString(
                                               projectDirectory.GetRelativePath(sourcePath).GetSystemPath())
                                          : QString::fromStdString(entry.mSourcePath);

      entry.mDestinationPath =
         exporter.GetExportPath(exportDirectory, exportSourcePath).toStdString();
      entry.mDocumentPtr = include.first;
      if (!entry.mDestinationPath.empty())
      {
         entries.push_back(entry);
      }
   }

   if (entries.empty())
   {
      result.mMessage = "The project does not include any files to export.";
      return result;
   }

   // 先整体检查冲突，避免导出到一半才失败
   std::vector<std::string> conflicts;
   for (const ExportEntry& entry : entries)
   {
      if (QFileInfo::exists(QString::fromStdString(entry.mDestinationPath)) && !overwrite)
      {
         conflicts.push_back(entry.mDestinationPath);
      }
   }
   if (!conflicts.empty())
   {
      std::string message = "Export aborted; destination file(s) already exist: ";
      for (size_t i = 0; (i < conflicts.size()) && (i < 5); ++i)
      {
         message += (i == 0 ? "" : ", ") + conflicts[i];
      }
      if (conflicts.size() > 5)
      {
         message += ", ...";
      }
      message += " (set 'overwrite' to true to replace them)";
      result.mMessage = message;
      return result;
   }

   size_t exportedCount = 0;
   size_t failedCount   = 0;
   for (const ExportEntry& entry : entries)
   {
      UtPath destination(entry.mDestinationPath);
      UtPath destinationDirectory = destination;
      destinationDirectory.Up();
      if (!QDir(QString::fromStdString(destinationDirectory.GetSystemPath())).mkpath("."))
      {
         ++failedCount;
         continue;
      }

      if (QFileInfo::exists(QString::fromStdString(entry.mDestinationPath)))
      {
         QFile::remove(QString::fromStdString(entry.mDestinationPath));
      }

      std::ofstream outfile(entry.mDestinationPath.c_str(), std::ios::binary);
      if (!outfile)
      {
         ++failedCount;
         continue;
      }
      entry.mDocumentPtr->WriteFile(outfile);
      outfile.close();
      if (!outfile)
      {
         ++failedCount;
         continue;
      }
      ++exportedCount;
   }

   if (failedCount > 0)
   {
      result.mMessage = "Exported " + std::to_string(exportedCount) + " file(s) to '" + directoryPath + "', but " +
                        std::to_string(failedCount) + " file(s) could not be written.";
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Exported " + std::to_string(exportedCount) + " file(s) to '" + directoryPath + "'.";
   return result;
}
} // namespace McpServer
