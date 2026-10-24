// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpFileToolSupport.hpp"

#include <cctype>
#include <fstream>

#include <QDir>
#include <QFileInfo>
#include <QString>

#include "Editor.hpp"
#include "EditorManager.hpp"
#include "Project.hpp"
#include "ProjectWorkspace.hpp"
#include "TextSource.hpp"
#include "TextSourceCache.hpp"
#include "UtPath.hpp"

namespace
{
//! 取得路径的规范化绝对形式；对不存在的文件使用其父目录的规范路径拼接文件名。
QString CanonicalPath(const QString& aPath)
{
   const QFileInfo info(aPath);
   const QString   canonical = info.canonicalFilePath();
   if (!canonical.isEmpty())
   {
      return QDir::cleanPath(canonical);
   }

   const QFileInfo parentInfo(info.absolutePath());
   const QString   parentCanonical = parentInfo.canonicalFilePath();
   if (!parentCanonical.isEmpty())
   {
      return QDir::cleanPath(QDir(parentCanonical).filePath(info.fileName()));
   }
   return QDir::cleanPath(info.absoluteFilePath());
}

QString ProjectRoot()
{
   wizard::Project* project = wizard::Project::Instance();
   return (project == nullptr) ? QString()
                               : CanonicalPath(QString::fromStdString(project->ProjectDirectory().GetSystemPath()));
}

bool IsSamePath(const QString& aLhs, const QString& aRhs)
{
#ifdef Q_OS_WIN
   return aLhs.compare(aRhs, Qt::CaseInsensitive) == 0;
#else
   return aLhs.compare(aRhs, Qt::CaseSensitive) == 0;
#endif
}

bool IsWithin(const QString& aRoot, const QString& aPath)
{
   if (aRoot.isEmpty() || aPath.isEmpty())
   {
      return false;
   }

#ifdef Q_OS_WIN
   const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
   const Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif

   if (aPath.compare(aRoot, sensitivity) == 0)
   {
      return true;
   }

   QString prefix = aRoot;
   if (!prefix.endsWith(QLatin1Char('/')))
   {
      prefix += QLatin1Char('/');
   }
   return aPath.startsWith(prefix, sensitivity);
}
} // namespace


namespace McpServer
{
namespace FileTools
{
std::string Argument(const ToolArguments& aArguments, const std::string& aName)
{
   const auto iter = aArguments.find(aName);
   return (iter == aArguments.end()) ? std::string() : iter->second;
}

bool ParseBoolean(const std::string& aValue, bool& aResult, std::string& aError)
{
   if (aValue.empty())
   {
      aResult = false;
      return true;
   }

   std::string lower(aValue);
   for (char& ch : lower)
   {
      ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
   }

   if ((lower == "true") || (lower == "1") || (lower == "yes"))
   {
      aResult = true;
      return true;
   }
   if ((lower == "false") || (lower == "0") || (lower == "no"))
   {
      aResult = false;
      return true;
   }

   aError = "Boolean argument must be 'true' or 'false'.";
   return false;
}

bool RequireProject(std::string& aError)
{
   wizard::Project*          project   = wizard::Project::Instance();
   wizard::ProjectWorkspace* workspace = wizard::ProjectWorkspace::Instance();
   if ((project == nullptr) || (workspace == nullptr) || (workspace->GetProject() == nullptr))
   {
      aError = "No project is open in AFSIM Wizard.";
      return false;
   }
   return true;
}

bool HasUnsavedChanges()
{
   wizard::ProjectWorkspace* workspace = wizard::ProjectWorkspace::Instance();
   wizard::Project*          project   = (workspace == nullptr) ? nullptr : workspace->GetProject();
   if (project == nullptr)
   {
      return false;
   }

   for (const auto& entry : project->GetSourceCache().GetSources())
   {
      if ((entry.second != nullptr) && entry.second->IsModified())
      {
         return true;
      }
   }
   return false;
}

std::string ResolveProjectPath(const std::string& aPath, std::string& aError)
{
   if (aPath.empty())
   {
      aError = "A file path is required.";
      return std::string();
   }

   wizard::Project* project = wizard::Project::Instance();
   if (project == nullptr)
   {
      aError = "No project is open in AFSIM Wizard.";
      return std::string();
   }

   const QString root = ProjectRoot();
   if (root.isEmpty())
   {
      aError = "The project directory is not available.";
      return std::string();
   }

   const QString path = QString::fromStdString(aPath);
   const QString candidate =
      QFileInfo(path).isAbsolute() ? path : QDir(root).filePath(path);
   const QString canonical = CanonicalPath(candidate);

   // 文件操作严格限制在项目目录内，防止通过绝对路径或 '..' 越出工作目录
   if (!IsWithin(root, canonical))
   {
      aError = "Path is outside the project directory '" + QDir::toNativeSeparators(root).toStdString() +
               "': " + QDir::toNativeSeparators(canonical).toStdString();
      return std::string();
   }

   return QDir::toNativeSeparators(canonical).toStdString();
}

bool IsInsideProject(const std::string& aAbsolutePath)
{
   const QString root = ProjectRoot();
   return IsWithin(root, CanonicalPath(QString::fromStdString(aAbsolutePath)));
}

bool PathsEqual(const std::string& aLhsPath, const std::string& aRhsPath)
{
   const QString lhs = CanonicalPath(QString::fromStdString(aLhsPath));
   const QString rhs = CanonicalPath(QString::fromStdString(aRhsPath));
   return IsSamePath(lhs, rhs);
}

bool CopyFileRaw(const std::string& aSourcePath, const std::string& aTargetPath, std::string& aError)
{
   std::ifstream infile(aSourcePath.c_str(), std::ios::binary);
   if (!infile)
   {
      aError = "Could not read source file: " + aSourcePath;
      return false;
   }

   std::ofstream outfile(aTargetPath.c_str(), std::ios::binary);
   if (!outfile)
   {
      aError = "Could not create target file: " + aTargetPath;
      return false;
   }

   outfile << infile.rdbuf();
   const bool readFailed = infile.bad();
   outfile.close();
   infile.close();

   if (readFailed || outfile.fail())
   {
      aError = "Could not copy file to: " + aTargetPath;
      return false;
   }
   return true;
}

bool PathExists(const std::string& aAbsolutePath)
{
   return UtPath(aAbsolutePath).Stat() != UtPath::cSTAT_ERROR;
}

bool EnsureParentDirectory(const std::string& aAbsolutePath, std::string& aError)
{
   UtPath directory(aAbsolutePath);
   directory.Up();
   if (directory.Stat() != UtPath::cDIRECTORY)
   {
      aError = "Directory does not exist: " + directory.GetSystemPath();
      return false;
   }
   return true;
}

wizard::TextSource* FindExistingSource(const std::string& aAbsolutePath)
{
   wizard::ProjectWorkspace* workspace = wizard::ProjectWorkspace::Instance();
   if (workspace == nullptr)
   {
      return nullptr;
   }
   return workspace->FindSource(UtPath(aAbsolutePath).GetSystemPath(), false, false);
}

wizard::TextSource* CurrentSource()
{
   wizard::ProjectWorkspace* workspace = wizard::ProjectWorkspace::Instance();
   if ((workspace == nullptr) || (workspace->GetEditorManager() == nullptr))
   {
      return nullptr;
   }

   wizard::Editor* editor = workspace->GetEditorManager()->GetCurrentEditor();
   return (editor == nullptr) ? nullptr : editor->GetSource();
}

bool EnsureLoaded(wizard::TextSource* aSourcePtr, std::string& aError)
{
   if (aSourcePtr == nullptr)
   {
      aError = "The file is not open in the project.";
      return false;
   }

   aSourcePtr->WaitForSourceModifications();
   if (!aSourcePtr->IsLoaded() && !aSourcePtr->Load())
   {
      aError = "Could not read file: " + aSourcePtr->GetFilePath().GetSystemPath();
      return false;
   }
   return true;
}
} // namespace FileTools
} // namespace McpServer
