// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSetEndTime.hpp"

#include "McpConfigToolSupport.hpp"
#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace McpServer
{
std::string SetEndTimeTool::Name() const
{
   return "set_end_time";
}

std::string SetEndTimeTool::Title() const
{
   return "配置仿真时长";
}

std::string SetEndTimeTool::Description() const
{
   return "设置仿真任务的总运行时长。脚本中已有顶层 end_time 命令时就地更新，"
          "否则追加到目标脚本末尾。";
}

std::vector<ToolParameter> SetEndTimeTool::Parameters() const
{
   return {
      {"time", "仿真总时长，包含单位，如 '1 hour'、'3600 s'。", true},
      {"file", "目标脚本文件，相对于项目目录。缺省使用主启动文件。", false},
   };
}

ToolTextResult SetEndTimeTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string time = ScenarioEdit::Argument(aArguments, "time");
   const std::string file = ScenarioEdit::Argument(aArguments, "file");

   std::string error;
   if (!ConfigTools::ValidateValue("time", time, error))
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

   const std::string command = "end_time " + time + "\n";
   if (!ConfigTools::UpsertCommand(source, "end_time", command, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Configured end_time in '" + target + "'.\n\n" + command;
   return result;
}
} // namespace McpServer
