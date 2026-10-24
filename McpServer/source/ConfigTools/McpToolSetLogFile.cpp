// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSetLogFile.hpp"

#include "McpConfigToolSupport.hpp"
#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace McpServer
{
std::string SetLogFileTool::Name() const
{
   return "set_log_file";
}

std::string SetLogFileTool::Title() const
{
   return "配置日志输出";
}

std::string SetLogFileTool::Description() const
{
   return "开启日志记录功能，并设置日志输出文件的存储路径。"
          "脚本中已有顶层 log_file 命令时就地更新，否则追加到目标脚本末尾。";
}

std::vector<ToolParameter> SetLogFileTool::Parameters() const
{
   return {
      {"path", "日志输出文件路径，写入脚本时原样使用，如 'output/CommTest.log'。", true},
      {"file", "目标脚本文件，相对于项目目录。缺省使用主启动文件。", false},
   };
}

ToolTextResult SetLogFileTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string path = ScenarioEdit::Argument(aArguments, "path");
   const std::string file = ScenarioEdit::Argument(aArguments, "file");

   std::string error;
   if (!ConfigTools::ValidateValue("path", path, error))
   {
      result.mMessage = error;
      return result;
   }

   wizard::TextSource* source = nullptr;
   std::string         target;
   if (!ConfigTools::ResolveTargetSource(file, source, target, error))
   {
      result.mMessage = error;
      return result;
   }

   const std::string command = "log_file " + path + "\n";
   if (!ConfigTools::UpsertCommand(source, "log_file", command, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Configured log_file in '" + target + "'.\n\n" + command;
   return result;
}
} // namespace McpServer
