// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSaveFile.hpp"

#include <string>

#include "McpFileToolSupport.hpp"
#include "TextSource.hpp"
#include "UtPath.hpp"

namespace McpServer
{
std::string SaveFileTool::Name() const
{
   return "save_file";
}

std::string SaveFileTool::Title() const
{
   return "Save File";
}

std::string SaveFileTool::Description() const
{
   return "保存已打开文件的内容。未提供 'file' 时保存当前活动编辑器中的文件；"
          "提供 'file' 时保存该文件（须已在项目中打开）。路径必须位于项目目录内。"
          "文件没有未保存修改时不会重写磁盘内容。";
}

std::vector<ToolParameter> SaveFileTool::Parameters() const
{
   return {{"file", "可选：要保存的文件路径；缺省保存当前活动文件", false}};
}

ToolTextResult SaveFileTool::Execute(const ToolArguments& aArguments) const
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
   std::string         absolutePath;
   if (fileArgument.empty())
   {
      sourcePtr = FileTools::CurrentSource();
      if (sourcePtr == nullptr)
      {
         result.mMessage = "No active editor. Open a file or pass the 'file' argument.";
         return result;
      }
      absolutePath = sourcePtr->GetFilePath().GetSystemPath();
   }
   else
   {
      absolutePath = FileTools::ResolveProjectPath(fileArgument, error);
      if (absolutePath.empty())
      {
         result.mMessage = error;
         return result;
      }
      sourcePtr = FileTools::FindExistingSource(absolutePath);
      if (sourcePtr == nullptr)
      {
         result.mMessage = "The file is not open in the project: " + absolutePath;
         return result;
      }
   }

   if (!FileTools::EnsureLoaded(sourcePtr, error))
   {
      result.mMessage = error;
      return result;
   }

   // 无未保存修改时保持磁盘内容不变，避免编辑器保存流程改写行尾或末尾空行
   if (!sourcePtr->IsModified() && FileTools::PathExists(absolutePath))
   {
      result.mSuccess = true;
      result.mMessage = "File '" + absolutePath + "' has no unsaved changes; disk content left unchanged.";
      return result;
   }

   if (!sourcePtr->Save())
   {
      result.mMessage = "Could not save file. Verify the file is writable: " + absolutePath;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Saved file '" + absolutePath + "'.";
   return result;
}
} // namespace McpServer
