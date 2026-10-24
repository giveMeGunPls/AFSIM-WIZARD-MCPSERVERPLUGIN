// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolClonePlatform.hpp"

#include <cctype>
#include <string>
#include <vector>

#include "McpScenarioEditSupport.hpp"
#include "Project.hpp"
#include "TextSource.hpp"
#include "WsfPM_Root.hpp"
#include "WsfPProxy.hpp"
#include "WsfPProxyObjectMap.hpp"

namespace
{
//! 找到以空白分隔出现的独立词，避免在注释或更长的标识符里误替换
size_t FindStandaloneToken(const std::string& aText, const std::string& aToken)
{
   size_t pos = 0;
   while ((pos = aText.find(aToken, pos)) != std::string::npos)
   {
      const bool leftOk =
         (pos == 0) || (std::isspace(static_cast<unsigned char>(aText[pos - 1])) != 0);
      const size_t after  = pos + aToken.size();
      const bool   rightOk =
         (after >= aText.size()) || (std::isspace(static_cast<unsigned char>(aText[after])) != 0);
      if (leftOk && rightOk)
      {
         return pos;
      }
      pos += aToken.size();
   }
   return std::string::npos;
}

bool PlatformExists(const std::string& aName)
{
   return WsfPM_Root(wizard::Project::Instance()->Proxy()).platforms().Find(aName).IsValid();
}
} // namespace

namespace McpServer
{
std::string ClonePlatformTool::Name() const
{
   return "clone_platform";
}

std::string ClonePlatformTool::Title() const
{
   return "克隆平台";
}

std::string ClonePlatformTool::Description() const
{
   return "复制已有平台的脚本定义，重命名副本，并追加到同一脚本文件。"
          "Wizard 重新解析脚本后，副本即成为想定的一部分。";
}

std::vector<ToolParameter> ClonePlatformTool::Parameters() const
{
   return {
      {"name", "要复制的平台名称。", true},
      {"new_name",
       "副本的名称。缺省依次使用 '<name>_1'、'<name>_2'……中第一个未被占用的名称。",
       false},
   };
}

ToolTextResult ClonePlatformTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string name    = ScenarioEdit::Argument(aArguments, "name");
   std::string       newName = ScenarioEdit::Argument(aArguments, "new_name");

   std::string     error;
   WsfPProxyNode   node;
   wizard::TextSource* source = nullptr;
   std::string     text;
   size_t          begin = 0;
   size_t          end   = 0;

   if (!ScenarioEdit::FindPlatform(name, node, error))
   {
      result.mMessage = error;
      return result;
   }
   if (!ScenarioEdit::GetDefinition(node, source, text, begin, end, error))
   {
      result.mMessage = error;
      return result;
   }

   if (newName.empty())
   {
      for (int index = 1;; ++index)
      {
         const std::string candidate = name + "_" + std::to_string(index);
         if (!PlatformExists(candidate))
         {
            newName = candidate;
            break;
         }
      }
   }
   else if (PlatformExists(newName))
   {
      result.mMessage = "A platform named '" + newName + "' already exists.";
      return result;
   }
   if ((newName.find_first_of(" \t\r\n/#") != std::string::npos) || (newName == name))
   {
      result.mMessage = "The clone name must differ from the original and must not contain spaces, '/', or '#'.";
      return result;
   }

   const size_t namePos = FindStandaloneToken(text, name);
   if (namePos == std::string::npos)
   {
      result.mMessage = "Unable to replace the platform name inside its definition.";
      return result;
   }
   text.replace(namePos, name.size(), newName);

   if (!ScenarioEdit::AppendText(source, "\n" + text, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Cloned platform '" + name + "' as '" + newName + "' in '" +
                     ScenarioEdit::RelativePath(source->GetFilePath().GetSystemPath()) + "'.\n\n" + text;
   return result;
}
} // namespace McpServer
