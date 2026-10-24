// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSaveFileAs.hpp"

#include <fstream>
#include <string>

#include "McpFileToolSupport.hpp"
#include "TextSource.hpp"
#include "UtPath.hpp"

namespace McpServer
{
std::string SaveFileAsTool::Name() const
{
   return "save_file_as";
}

std::string SaveFileAsTool::Title() const
{
   return "另存文件";
}

std::string SaveFileAsTool::Description() const
{
   return "把文件内容写入新路径（另存为）。未提供 'file' 时复制当前活动文件，"
          "提供 'file' 时复制该文件。路径必须位于项目目录内。原文件仍是编辑器绑定的文件。"
          "文件没有未保存修改时按原始字节复制，保留行尾与首尾空白；"
          "目标文件已存在时须显式设置 'overwrite' 为 true。";
}

std::vector<ToolParameter> SaveFileAsTool::Parameters() const
{
   return {{"file", "可选：要另存的源文件路径；缺省使用当前活动文件", false},
           {"newFile", "目标文件路径（绝对路径或相对于项目目录）", true},
           {"overwrite", "目标文件已存在时是否覆盖，默认 false", false}};
}

ToolTextResult SaveFileAsTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   const std::string   fileArgument = FileTools::Argument(aArguments, "file");
   wizard::TextSource* sourcePtr    = nullptr;
   std::string         sourcePath;
   if (fileArgument.empty())
   {
      sourcePtr = FileTools::CurrentSource();
      if (sourcePtr == nullptr)
      {
         result.mMessage = "No active editor. Open a file or pass the 'file' argument.";
         return result;
      }
      sourcePath = sourcePtr->GetFilePath().GetSystemPath();
   }
   else
   {
      sourcePath = FileTools::ResolveProjectPath(fileArgument, error);
      if (sourcePath.empty())
      {
         result.mMessage = error;
         return result;
      }
      sourcePtr = FileTools::FindExistingSource(sourcePath);
      if (sourcePtr == nullptr)
      {
         result.mMessage = "The file is not open in the project: " + sourcePath;
         return result;
      }
   }

   const std::string targetPath = FileTools::ResolveProjectPath(FileTools::Argument(aArguments, "newFile"), error);
   if (targetPath.empty())
   {
      result.mMessage = error;
      return result;
   }

   bool              overwrite = false;
   const std::string overwriteArgument = FileTools::Argument(aArguments, "overwrite");
   if (!FileTools::ParseBoolean(overwriteArgument, overwrite, error))
   {
      result.mMessage = error;
      return result;
   }

   if (FileTools::PathExists(targetPath) && !overwrite)
   {
      result.mMessage = "Target file already exists: " + targetPath + " (set 'overwrite' to true to replace it)";
      return result;
   }

   if (!FileTools::EnsureParentDirectory(targetPath, error))
   {
      result.mMessage = error;
      return result;
   }

   if (!FileTools::EnsureLoaded(sourcePtr, error))
   {
      result.mMessage = error;
      return result;
   }

   const bool hasUnsavedChanges = sourcePtr->IsModified();
   const bool sameFile          = FileTools::PathsEqual(sourcePath, targetPath);
   if (sameFile)
   {
      if (!hasUnsavedChanges)
      {
         result.mSuccess = true;
         result.mMessage = "Source and target are the same file and it has no unsaved changes; left unchanged.";
         return result;
      }

      if (!sourcePtr->Save())
      {
         result.mMessage = "Could not save file. Verify the file is writable: " + sourcePath;
         return result;
      }
   }
   else if (!hasUnsavedChanges)
   {
      if (!FileTools::CopyFileRaw(sourcePath, targetPath, error))
      {
         result.mMessage = error;
         return result;
      }
   }
   else
   {
      std::ofstream outfile(targetPath.c_str(), std::ios::binary);
      if (!outfile)
      {
         result.mMessage = "Could not create target file: " + targetPath;
         return result;
      }
      sourcePtr->WriteFile(outfile);
      outfile.close();
      if (!outfile)
      {
         result.mMessage = "Could not write target file: " + targetPath;
         return result;
      }
   }

   result.mSuccess = true;
   result.mMessage = sameFile ? ("Saved file '" + sourcePath + "'.")
                              : ("Saved a copy of '" + sourcePath + "' to '" + targetPath + "'.");
   return result;
}
} // namespace McpServer
