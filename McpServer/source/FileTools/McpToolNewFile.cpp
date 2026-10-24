// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolNewFile.hpp"

#include <fstream>
#include <string>

#include "McpFileToolSupport.hpp"
#include "Project.hpp"

namespace McpServer
{
std::string NewFileTool::Name() const
{
   return "new_file";
}

std::string NewFileTool::Title() const
{
   return "New File";
}

std::string NewFileTool::Description() const
{
   return "创建文本文件并加入当前项目。'file' 可以是绝对路径，或相对于项目目录的路径，"
          "但必须位于项目目录内。未提供 'content' 时创建空文件。";
}

std::vector<ToolParameter> NewFileTool::Parameters() const
{
   return {{"file", "新文件的路径（绝对路径或相对于项目目录，须位于项目目录内）", true},
           {"content", "可选的初始文件内容；缺省时创建空文件，内容按原始字节写入", false, true}};
}

ToolTextResult NewFileTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   // 新建文件依赖项目缓存与文件树，必须先打开项目
   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   const std::string absolutePath = FileTools::ResolveProjectPath(FileTools::Argument(aArguments, "file"), error);
   if (absolutePath.empty())
   {
      result.mMessage = error;
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

   // 直接按原始字节写入，避免编辑器模板的模态对话框和行尾规范化
   const std::string content = FileTools::Argument(aArguments, "content");
   std::ofstream     outfile(absolutePath.c_str(), std::ios::binary);
   if (!outfile)
   {
      result.mMessage = "Could not create file: " + absolutePath;
      return result;
   }
   if (!content.empty())
   {
      outfile.write(content.data(), static_cast<std::streamsize>(content.size()));
   }
   outfile.close();
   if (!outfile)
   {
      result.mMessage = "Could not write file: " + absolutePath;
      return result;
   }

   wizard::Project* project = wizard::Project::Instance();
   if (!project->OpenFileInTextEditor(absolutePath))
   {
      result.mMessage = "Created file '" + absolutePath + "' but could not open it in the editor.";
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Created file '" + absolutePath + "' and opened it in the editor.";
   return result;
}
} // namespace McpServer
