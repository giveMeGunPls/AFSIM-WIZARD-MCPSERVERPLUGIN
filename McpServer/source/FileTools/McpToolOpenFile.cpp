// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolOpenFile.hpp"

#include <string>

#include <QString>

#include "McpFileToolSupport.hpp"
#include "Project.hpp"
#include "UtPath.hpp"
#include "Util.hpp"

namespace McpServer
{
std::string OpenFileTool::Name() const
{
   return "open_file";
}

std::string OpenFileTool::Title() const
{
   return "Open File";
}

std::string OpenFileTool::Description() const
{
   return "把项目中的文本文件打开到编辑器。'file' 可以是绝对路径，或相对于项目目录的路径，"
          "但必须位于项目目录内。仅支持文本文件，非文本文件请使用系统外部工具打开。";
}

std::vector<ToolParameter> OpenFileTool::Parameters() const
{
   return {{"file", "要打开的文件路径（绝对路径或相对于项目目录）", true}};
}

ToolTextResult OpenFileTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

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

   const UtPath filePath(absolutePath);
   if (filePath.Stat() == UtPath::cDIRECTORY)
   {
      result.mMessage = "Path is a directory, not a file: " + absolutePath;
      return result;
   }
   if (filePath.Stat() != UtPath::cFILE)
   {
      result.mMessage = "File does not exist: " + absolutePath;
      return result;
   }

   // 与 File > Open File 的文本编辑分支一致，仅接受文本文件
   if (wizard::Util::IsNonTextFile(QString::fromStdString(absolutePath)))
   {
      result.mMessage = "Only text files can be opened in the editor: " + absolutePath;
      return result;
   }

   wizard::Project* project = wizard::Project::Instance();
   if (!project->OpenFileInTextEditor(absolutePath))
   {
      result.mMessage = "Could not open file in the editor: " + absolutePath;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Opened file '" + absolutePath + "' in the editor.";
   return result;
}
} // namespace McpServer
