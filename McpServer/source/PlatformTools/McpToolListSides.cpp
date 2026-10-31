// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolListSides.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "UtColor.hpp"

namespace
{
//! 把名称列表拼成一行，用于错误提示
std::string JoinNames(const std::vector<std::string>& aNames)
{
   std::string text;
   for (size_t i = 0; i < aNames.size(); ++i)
   {
      if (i != 0)
      {
         text += ", ";
      }
      text += aNames[i];
   }
   return text;
}
} // namespace

namespace McpServer
{
std::string ListSidesTool::Name() const
{
   return "list_sides";
}

std::string ListSidesTool::Title() const
{
   return "列出阵营";
}

std::string ListSidesTool::Description() const
{
   return "列出 AFSIM Wizard 平台数据面板中可选的阵营名称，"
          "阵营名取自颜色表，可直接用作平台脚本中的 side 属性值；"
          "可通过 side 只查询其中一个阵营。";
}

std::vector<ToolParameter> ListSidesTool::Parameters() const
{
   return {
      {"side", "阵营（颜色）名称，如 'blue'；省略时输出全部阵营。", false},
   };
}

ToolTextResult ListSidesTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   try
   {
      const std::string side = ScenarioEdit::Argument(aArguments, "side");

      const std::vector<std::string> sides = CollectSideNames();
      if (sides.empty())
      {
         result.mMessage = "No side is defined in AFSIM Wizard.";
         return result;
      }

      std::vector<std::string> selected;
      if (side.empty())
      {
         selected = sides;
      }
      else if (std::find(sides.begin(), sides.end(), side) != sides.end())
      {
         selected.push_back(side);
      }
      else
      {
         result.mMessage = "Unknown side '" + side + "'. Available sides are: " + JoinNames(sides) + ".";
         return result;
      }

      std::string text = "Sides (" + std::to_string(selected.size()) + "):\n";
      for (const std::string& name : selected)
      {
         text += "  " + name + "\n";
      }

      result.mSuccess = true;
      result.mMessage = text;
      return result;
   }
   catch (const std::exception& aError)
   {
      result.mSuccess = false;
      result.mMessage = std::string("list_sides failed: ") + aError.what();
   }
   catch (...)
   {
      result.mSuccess = false;
      result.mMessage = "list_sides failed with an unknown error.";
   }
   return result;
}

std::vector<std::string> ListSidesTool::CollectSideNames()
{
   std::vector<std::string> names;
   for (const std::pair<const std::string, UtColor>& entry : UtColor::GetColorMap())
   {
      names.push_back(entry.first);
   }

   // 颜色表哈希顺序不稳定，排序并去重后保证工具输出可复现
   std::sort(names.begin(), names.end());
   names.erase(std::unique(names.begin(), names.end()), names.end());
   return names;
}
} // namespace McpServer
