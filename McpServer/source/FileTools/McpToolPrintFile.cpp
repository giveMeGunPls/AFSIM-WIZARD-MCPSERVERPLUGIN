// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolPrintFile.hpp"

#include <string>

#include <QPrinter>
#include <QString>
#include <QTextDocument>

#include "Editor.hpp"
#include "McpFileToolSupport.hpp"
#include "ProjectWorkspace.hpp"
#include "TextSource.hpp"
#include "UtPath.hpp"

namespace McpServer
{
std::string PrintFileTool::Name() const
{
   return "print_file";
}

std::string PrintFileTool::Title() const
{
   return "Print File";
}

std::string PrintFileTool::Description() const
{
   return "打印文件内容。提供 'output' 时输出为 PDF 文件（无需打印机）；未提供时发送到默认打印机。"
          "未提供 'file' 时打印当前活动文件。路径必须位于项目目录内。";
}

std::vector<ToolParameter> PrintFileTool::Parameters() const
{
   return {{"file", "可选：要打印的文件路径；缺省使用当前活动文件", false},
           {"output", "可选：输出的 PDF 路径；缺省使用系统默认打印机", false}};
}

ToolTextResult PrintFileTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   const std::string fileArgument = FileTools::Argument(aArguments, "file");
   wizard::TextSource* sourcePtr  = nullptr;
   if (fileArgument.empty())
   {
      sourcePtr = FileTools::CurrentSource();
      if (sourcePtr == nullptr)
      {
         result.mMessage = "No active editor. Open a file or pass the 'file' argument.";
         return result;
      }
   }
   else
   {
      const std::string absolutePath = FileTools::ResolveProjectPath(fileArgument, error);
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

   wizard::ProjectWorkspace* workspace = wizard::ProjectWorkspace::Instance();
   wizard::Editor*           editor    = workspace->OpenSource(sourcePtr);
   if (editor == nullptr)
   {
      result.mMessage = "Could not open the file in an editor for printing: " +
                        sourcePtr->GetFilePath().GetSystemPath();
      return result;
   }

   const std::string outputArgument = FileTools::Argument(aArguments, "output");
   QPrinter          printer(QPrinter::HighResolution);
   if (outputArgument.empty())
   {
      printer.setOutputFormat(QPrinter::NativeFormat);
      if (!printer.isValid())
      {
         result.mMessage = "No default printer is available. Pass the 'output' argument to write a PDF instead.";
         return result;
      }
      editor->document()->print(&printer);

      result.mSuccess = true;
      result.mMessage = "Sent '" + sourcePtr->GetFilePath().GetSystemPath() + "' to the default printer.";
      return result;
   }

   const std::string absoluteOutput = FileTools::ResolveProjectPath(outputArgument, error);
   if (absoluteOutput.empty())
   {
      result.mMessage = error;
      return result;
   }
   if (!FileTools::EnsureParentDirectory(absoluteOutput, error))
   {
      result.mMessage = error;
      return result;
   }

   printer.setOutputFormat(QPrinter::PdfFormat);
   printer.setOutputFileName(QString::fromStdString(absoluteOutput));
   editor->document()->print(&printer);

   result.mSuccess = true;
   result.mMessage = "Printed '" + sourcePtr->GetFilePath().GetSystemPath() + "' to PDF '" + absoluteOutput + "'.";
   return result;
}
} // namespace McpServer
