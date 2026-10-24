// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSetEventOutput.hpp"

#include "McpConfigToolSupport.hpp"
#include "McpScenarioEditSupport.hpp"
#include "TextSource.hpp"

namespace McpServer
{
std::string SetEventOutputTool::Name() const
{
   return "set_event_output";
}

std::string SetEventOutputTool::Title() const
{
   return "配置事件记录";
}

std::string SetEventOutputTool::Description() const
{
   return "开启事件记录功能，并定义事件记录文件的存储路径。"
          "脚本中已有顶层 event_output 块时只更新其 file 行并保留其他事件设置，"
          "否则追加完整块。";
}

std::vector<ToolParameter> SetEventOutputTool::Parameters() const
{
   return {
      {"path", "事件记录文件路径，写入脚本时原样使用，如 'output/CommTest.evt'。", true},
      {"file", "目标脚本文件，相对于项目目录。缺省使用主启动文件。", false},
   };
}

ToolTextResult SetEventOutputTool::Execute(const ToolArguments& aArguments) const
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

   const std::string indent = ScenarioEdit::IndentUnit();
   const std::string block  = "event_output\n" + indent + "file " + path + "\n" + "end_event_output\n";
   const std::vector<ConfigTools::BlockOption> options = {{"file", "file " + path}};

   if (!ConfigTools::UpsertBlock(source, "event_output", "end_event_output", block, options, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Configured event_output in '" + target + "'.\n\n" + block;
   return result;
}
} // namespace McpServer
