// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolListIcons.hpp"

#include <exception>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "VaModelDatabase.hpp"
#include "WkfVtkEnvironment.hpp"

namespace
{
//! 未设置分类时使用的分组名，与 VaModelDatabase 读取模型定义时的默认分类一致
const char* const kUncategorizedLabel = "uncategorized";

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
std::string ListIconsTool::Name() const
{
   return "list_icons";
}

std::string ListIconsTool::Title() const
{
   return "列出图标";
}

std::string ListIconsTool::Description() const
{
   return "按分类列出 AFSIM Wizard 模型数据库中的全部图标（icon）名称，"
          "图标名可直接用作平台脚本中的 icon 属性值；未设置分类的图标归入 "
          "uncategorized。可通过 category 只查询其中一类。";
}

std::vector<ToolParameter> ListIconsTool::Parameters() const
{
   return {
      {"category", "图标分类名称，如 'aircraft'；省略时输出全部图标。", false},
   };
}

ToolTextResult ListIconsTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   try
   {
      const std::string category = ScenarioEdit::Argument(aArguments, "category");

      if (!wkf::VtkEnvironment::Exists())
      {
         result.mMessage = "The AFSIM Wizard visualization environment is not available.";
         return result;
      }

      const std::vector<IconGroup> groups = CollectIconGroups();
      if (groups.empty())
      {
         result.mMessage = "No icon definition is available in the AFSIM Wizard model database.";
         return result;
      }

      if (!category.empty())
      {
         bool                     found      = false;
         std::vector<std::string> categories;
         for (const IconGroup& group : groups)
         {
            categories.push_back(group.mCategory);
            if (group.mCategory == category)
            {
               found = true;
            }
         }
         if (!found)
         {
            result.mMessage =
               "Unknown icon category '" + category + "'. Available categories are: " + JoinNames(categories) + ".";
            return result;
         }
      }

      std::string text;
      for (const IconGroup& group : groups)
      {
         if (!category.empty() && (group.mCategory != category))
         {
            continue;
         }

         text += group.mCategory + " icons (" + std::to_string(group.mNames.size()) + "):\n";
         for (const std::string& name : group.mNames)
         {
            text += "  " + name + "\n";
         }
         text += "\n";
      }

      result.mSuccess = true;
      result.mMessage = text;
      return result;
   }
   catch (const std::exception& aError)
   {
      result.mSuccess = false;
      result.mMessage = std::string("list_icons failed: ") + aError.what();
   }
   catch (...)
   {
      result.mSuccess = false;
      result.mMessage = "list_icons failed with an unknown error.";
   }
   return result;
}

std::vector<ListIconsTool::IconGroup> ListIconsTool::CollectIconGroups()
{
   std::vector<IconGroup> groups;

   vespa::VaModelDatabase* database = vaEnv.GetModelDatabase();
   if (!database)
   {
      return groups;
   }

   // GetNameList 已是 std::set，按名称升序；std::map 让分类也按名称升序，
   // 与 PlatformData 插件里 model->sort(0) 的排序结果一致
   std::map<std::string, std::vector<std::string>> grouped;
   std::set<std::string>                           nameList = database->GetNameList();
   for (const std::string& name : nameList)
   {
      const vespa::VaModelDefinition* definition = database->GetModelDefinition(name);
      if (!definition)
      {
         continue;
      }

      const std::string category = definition->GetCategories();
      grouped[category.empty() ? std::string(kUncategorizedLabel) : category].push_back(name);
   }

   for (const std::pair<const std::string, std::vector<std::string>>& entry : grouped)
   {
      IconGroup group;
      group.mCategory = entry.first;
      group.mNames    = entry.second;
      groups.push_back(std::move(group));
   }
   return groups;
}
} // namespace McpServer
