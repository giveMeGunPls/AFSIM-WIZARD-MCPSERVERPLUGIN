// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSetEventPipe.hpp"

#include "McpConfigToolSupport.hpp"
#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace McpServer
{
std::string SetEventPipeTool::Name() const
{
   return "set_event_pipe";
}

std::string SetEventPipeTool::Title() const
{
   return "配置事件回放";
}

std::string SetEventPipeTool::Description() const
{
   return "开启事件回放功能，并定义回放文件的存储路径。"
          "脚本中已有顶层 event_pipe 块时只更新 use_preset 与 file 行并保留其他设置，"
          "否则追加完整块。";
}

std::vector<ToolParameter> SetEventPipeTool::Parameters() const
{
   return {
      {"path", "回放文件路径，写入脚本时原样使用，如 'output/CommTest.aer'。", true},
      {"preset", "回放预设级别，缺省为 'full'。", false},
      {"file", "目标脚本文件，相对于项目目录。缺省使用主启动文件。", false},
   };
}

ToolTextResult SetEventPipeTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string path   = ScenarioEdit::Argument(aArguments, "path");
   const std::string preset = ScenarioEdit::Argument(aArguments, "preset");
   const std::string file   = ScenarioEdit::Argument(aArguments, "file");

   std::string error;
   if (!ConfigTools::ValidateValue("path", path, error))
   {
      result.mMessage = error;
      return result;
   }
   const std::string level = preset.empty() ? "full" : preset;
   if (!ConfigTools::ValidateValue("preset", level, error))
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

   const std::string indent = ScenarioEdit::IndentUnit();
   const std::string block  = "event_pipe\n" + indent + "use_preset " + level + "\n" + indent + "file " + path + "\n" +
                             "end_event_pipe\n";
   const std::vector<ConfigTools::BlockOption> options = {
      {"use_preset", "use_preset " + level},
      {"file", "file " + path},
   };

   if (!ConfigTools::UpsertBlock(source, "event_pipe", "end_event_pipe", block, options, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Configured event_pipe in '" + target + "'.\n\n" + block;
   return result;
}
} // namespace McpServer
