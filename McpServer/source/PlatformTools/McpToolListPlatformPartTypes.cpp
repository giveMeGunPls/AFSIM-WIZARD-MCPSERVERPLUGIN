// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolListPlatformPartTypes.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "Project.hpp"
#include "WsfPM_ObjectMap.hpp"
#include "WsfPM_Root.hpp"
#include "WsfPProxy.hpp"
#include "WsfPProxyNode.hpp"

namespace
{
//! 部件类别与基础类型树分类名的对应关系，顺序即输出顺序
const std::vector<std::pair<std::string, std::string>>& PartKindCategories()
{
   static const std::vector<std::pair<std::string, std::string>> categories = {
      {"sensor", "sensorType"},
      {"weapon", "weaponType"},
      {"comm", "commType"},
      {"processor", "processorType"},
      {"mover", "moverType"},
      {"fuel", "fuelType"}};
   return categories;
}

//! 收集某一类别下脚本定义与内置的部件类型名
void CollectTypeNames(const WsfPProxyNode& aRoot, const std::string& aCategory, std::vector<std::string>& aNames)
{
   WsfPM_ObjectMap            map(aRoot + aCategory);
   std::vector<WsfPProxyNode> nodes;
   map.GetAllValues(nodes);

   for (const WsfPProxyNode& node : nodes)
   {
      aNames.push_back(node.GetName());
   }

   std::sort(aNames.begin(), aNames.end());
   aNames.erase(std::unique(aNames.begin(), aNames.end()), aNames.end());
}
} // namespace

namespace McpServer
{
std::string ListPlatformPartTypesTool::Name() const
{
   return "list_platform_part_types";
}

std::string ListPlatformPartTypesTool::Title() const
{
   return "列出平台部件类型";
}

std::string ListPlatformPartTypesTool::Description() const
{
   return "列出平台部件可用的类型，包含脚本定义的类型与内置类型，"
          "按 sensor、weapon、comm、processor、mover、fuel 分类输出；"
          "可通过 kind 只查询其中一类。";
}

std::vector<ToolParameter> ListPlatformPartTypesTool::Parameters() const
{
   return {
      {"kind", "部件类别：sensor、weapon、comm、processor、mover 或 fuel；省略时输出全部类别。", false},
   };
}

ToolTextResult ListPlatformPartTypesTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;

   wizard::Project* project = wizard::Project::Instance();
   WsfPProxy*       proxy   = project ? project->Proxy() : nullptr;
   if (!proxy)
   {
      result.mMessage = "No project is open in AFSIM Wizard, or the scenario has not been parsed yet.";
      return result;
   }

   const std::string kind = ScenarioEdit::Argument(aArguments, "kind");

   bool selected = kind.empty();
   for (const std::pair<std::string, std::string>& category : PartKindCategories())
   {
      if (kind == category.first)
      {
         selected = true;
      }
   }
   if (!selected)
   {
      result.mMessage = "Unknown part kind '" + kind +
                        "'. Use sensor, weapon, comm, processor, mover, or fuel.";
      return result;
   }

   const WsfPProxyNode root = WsfPM_Root(proxy);
   std::string         text;
   for (const std::pair<std::string, std::string>& category : PartKindCategories())
   {
      if (!kind.empty() && (kind != category.first))
      {
         continue;
      }

      std::vector<std::string> names;
      CollectTypeNames(root, category.second, names);

      text += category.first + " types (" + std::to_string(names.size()) + "):\n";
      if (names.empty())
      {
         text += "  (none)\n";
      }
      for (const std::string& name : names)
      {
         text += "  " + name + "\n";
      }
      text += "\n";
   }

   result.mSuccess = true;
   result.mMessage = text;
   return result;
}
} // namespace McpServer
