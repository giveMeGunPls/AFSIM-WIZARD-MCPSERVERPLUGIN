// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolListPlatformTypes.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "McpScenarioQuery.hpp"
#include "Project.hpp"
#include "UtPath.hpp"
#include "WsfPM_Root.hpp"
#include "WsfPProxy.hpp"
#include "WsfPProxyIndex.hpp"
#include "WsfPProxyObjectMap.hpp"
#include "WsfPProxyRegistry.hpp"

namespace McpServer
{
std::string ListPlatformTypesTool::Name() const
{
   return "list_platform_types";
}

std::string ListPlatformTypesTool::Title() const
{
   return "List platform types";
}

std::string ListPlatformTypesTool::Description() const
{
   return "List platform types available in the scenario currently open in AFSIM Wizard, "
          "including built-in types and script-defined platform_type names, plus existing "
          "platform names and the default target script file.";
}

ToolTextResult ListPlatformTypesTool::Execute(const ToolArguments&) const
{
   ToolTextResult result;

   wizard::Project* project = wizard::Project::Instance();
   WsfPProxy*       proxy   = project ? project->Proxy() : nullptr;
   if (!proxy)
   {
      result.mMessage = "No project is open in AFSIM Wizard, or the scenario has not been parsed yet.";
      return result;
   }

   std::vector<std::string> builtinTypes;
   CollectBasicTypeNames(proxy, "platformType", builtinTypes);

   std::vector<WsfPProxyNode> typeNodes;
   WsfPM_Root(proxy).platformTypes().GetAllValues(typeNodes);
   std::vector<std::string> scriptTypes;
   for (const WsfPProxyNode& node : typeNodes)
   {
      scriptTypes.push_back(node.GetName());
   }

   std::vector<WsfPProxyNode> platformNodes;
   WsfPM_Root(proxy).platforms().GetAllValues(platformNodes);
   std::vector<std::string> platforms;
   for (const WsfPProxyNode& node : platformNodes)
   {
      platforms.push_back(node.GetName());
   }

   SortUnique(builtinTypes);
   SortUnique(scriptTypes);
   SortUnique(platforms);

   const std::string targetFile = ScenarioQuery::DefaultTargetFile();
   std::string       text       = "Project: " + project->ProjectDirectory().GetSystemPath() + "\n";
   text += "Default target file: " + (targetFile.empty() ? std::string("(none)") : targetFile) + "\n\n";

   AppendSection(text, "Built-in platform types", builtinTypes);
   AppendSection(text, "Script-defined platform types", scriptTypes);
   AppendSection(text, "Existing platforms", platforms);

   result.mSuccess = true;
   result.mMessage = text;
   return result;
}

void ListPlatformTypesTool::CollectBasicTypeNames(WsfPProxy*                 aProxy,
                                                  const char*                aCategory,
                                                  std::vector<std::string>& aNames)
{
   if (!aProxy)
   {
      return;
   }

   const std::shared_ptr<WsfPProxyRegistry>& registry = aProxy->GetRegistry();
   if (!registry)
   {
      return;
   }

   WsfPProxyValue      categoryValue = registry->GetBasicRoot().GetAttr(aCategory);
   WsfPProxyObjectMap* objectMapPtr  = categoryValue.GetObjectMap();
   if (!objectMapPtr)
   {
      return;
   }

   for (const std::pair<const std::string, WsfPProxyValue>& entry : objectMapPtr->GetValues())
   {
      aNames.push_back(entry.first);
   }
}

void ListPlatformTypesTool::SortUnique(std::vector<std::string>& aValues)
{
   std::sort(aValues.begin(), aValues.end());
   aValues.erase(std::unique(aValues.begin(), aValues.end()), aValues.end());
}

void ListPlatformTypesTool::AppendSection(std::string&                    aText,
                                          const char*                     aTitle,
                                          const std::vector<std::string>& aValues)
{
   aText += aTitle;
   aText += " (" + std::to_string(aValues.size()) + "):\n";
   if (aValues.empty())
   {
      aText += "  (none)\n";
   }
   for (const std::string& value : aValues)
   {
      aText += "  " + value + "\n";
   }
   aText += "\n";
}
} // namespace McpServer
