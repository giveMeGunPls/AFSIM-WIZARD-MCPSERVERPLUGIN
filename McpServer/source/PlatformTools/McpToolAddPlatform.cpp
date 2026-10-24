// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolAddPlatform.hpp"

#include <cctype>
#include <string>
#include <utility>
#include <vector>

#include <QString>

#include "EditorPrefObject.hpp"
#include "Environment.hpp"
#include "FileTools/McpFileToolSupport.hpp"
#include "McpScenarioQuery.hpp"
#include "Project.hpp"
#include "ProjectWorkspace.hpp"
#include "TextSource.hpp"
#include "WkfEnvironment.hpp"
#include "WsfPM_Root.hpp"
#include "WsfPProxy.hpp"
#include "WsfPProxyObjectMap.hpp"

namespace
{
//! 读取字符串参数；未提供时返回空串
std::string Argument(const McpServer::ToolArguments& aArguments, const char* aName)
{
   const McpServer::ToolArguments::const_iterator iter = aArguments.find(aName);
   return (iter == aArguments.end()) ? std::string() : iter->second;
}

//! 未显式写出半球字母时补默认值：纬度北纬 N、经度东经 E
std::string WithDefaultHemisphere(const std::string& aValue, char aDefault)
{
   if (aValue.empty())
   {
      return aValue;
   }

   const char last = static_cast<char>(std::tolower(static_cast<unsigned char>(aValue.back())));
   if ((last == 'n') || (last == 's') || (last == 'e') || (last == 'w'))
   {
      return aValue;
   }
   return aValue + aDefault;
}
} // namespace

namespace McpServer
{
std::string AddPlatformTool::Name() const
{
   return "add_platform";
}

std::string AddPlatformTool::Title() const
{
   return "添加平台";
}

std::string AddPlatformTool::Description() const
{
   return "把平台定义追加到 AFSIM Wizard 当前打开的想定的脚本文件中。"
          "Wizard 重新解析脚本后，该平台即成为想定的一部分。";
}

std::vector<ToolParameter> AddPlatformTool::Parameters() const
{
   return {
      {"name", "唯一的平台名称（不能包含空格、'/' 或 '#'）。", true},
      {"type", "想定中已存在的平台类型名称。", true},
      {"file",
       "目标脚本文件，可为绝对路径或相对于项目目录的路径。"
       "缺省使用包含平台定义最多的文件。",
       false},
      {"latitude",
       "AFSIM 格式的纬度，如 '27:52:46.72' 或 '27.8'。须同时提供经度。",
       false},
      {"longitude",
       "AFSIM 格式的经度，如 '118:01:01.03' 或 '118.0'。须同时提供纬度。",
       false},
      {"side", "阵营名称，如 'blue'。", false},
      {"icon", "图标名称，如 'F-16'。", false},
      {"altitude", "带单位的高度，如 '10000 m'。", false},
      {"heading", "带单位的航向，如 '90 deg'。", false},
      {"pitch", "带单位的俯仰角，如 '0 deg'。", false},
      {"roll", "带单位的滚转角，如 '0 deg'。", false},
   };
}

ToolTextResult AddPlatformTool::Execute(const ToolArguments& aArguments) const
{
   Request     request;
   std::string error;
   ToolTextResult result;
   if (!ParseRequest(aArguments, request, error))
   {
      result.mMessage = error;
      return result;
   }

   wizard::Project* project = wizard::Project::Instance();
   WsfPProxy*       proxy   = project ? project->Proxy() : nullptr;
   if (!proxy)
   {
      result.mMessage = "No project is open in AFSIM Wizard, or the scenario has not been parsed yet.";
      return result;
   }

   if (WsfPM_Root(proxy).platforms().Exists(request.mName))
   {
      result.mMessage = "A platform named '" + request.mName + "' already exists.";
      return result;
   }

   // proxy 只暴露脚本定义的 platform_type；脚本里没有时再查基础类型树，允许 WSF_PLATFORM 等内置类型
   WsfPProxyNode typeNode = WsfPM_Root(proxy).platformTypes().Find(request.mType);
   if (!typeNode.IsValid())
   {
      typeNode = WsfPM_Root(proxy).platformTypes() + request.mType;
      if (!typeNode.SwitchToBasicTypes())
      {
         result.mMessage = "Platform type '" + request.mType + "' does not exist in this scenario.";
         return result;
      }
   }

   const std::string relativeFile =
      request.mFile.empty() ? ScenarioQuery::DefaultTargetFile() : request.mFile;
   if (relativeFile.empty())
   {
      result.mMessage = "No target script file is available in this project; pass 'file' explicitly.";
      return result;
   }

   // 目标脚本文件必须位于项目目录内，避免通过绝对路径或 '..' 写到项目外
   std::string       pathError;
   const std::string absoluteFile = FileTools::ResolveProjectPath(relativeFile, pathError);
   if (absoluteFile.empty())
   {
      result.mMessage = pathError;
      return result;
   }

   wizard::TextSource* source = wizard::ProjectWorkspace::Instance()->FindSource(absoluteFile, false, false);
   if (!source)
   {
      result.mMessage = "Unable to find the target script file: " + relativeFile;
      return result;
   }

   // 只拼接脚本片段，场景中的平台由解析器随后生成
   const std::string indent      = AttributeIndent();
   std::string       platformDef = "\nplatform " + request.mName + " " + request.mType + "\n";

   // 固定顺序：side、icon 在最前，position 第三
   AppendAttribute(platformDef, indent, "side", request.mSide);
   AppendAttribute(platformDef, indent, "icon", request.mIcon);

   if (!request.mLatitude.empty())
   {
      platformDef += indent + "position " + request.mLatitude + " " + request.mLongitude + "\n";
   }

   AppendAttribute(platformDef, indent, "altitude", request.mAltitude);
   AppendAttribute(platformDef, indent, "heading", request.mHeading);
   AppendAttribute(platformDef, indent, "pitch", request.mPitch);
   AppendAttribute(platformDef, indent, "roll", request.mRoll);

   platformDef += "end_platform\n";

   if (source->GetViews().empty())
   {
      source->NewView(project);
   }
   source->Append(platformDef);

   wizEnv.RequestSelectPlatform(QString::fromStdString(request.mName));

   result.mSuccess = true;
   result.mMessage = "Created platform '" + request.mName + "' (type '" + request.mType + "') in '" + relativeFile +
                     "'.\n\n" + platformDef;
   return result;
}

bool AddPlatformTool::ParseRequest(const ToolArguments& aArguments, Request& aRequest, std::string& aError)
{
   aRequest.mName      = Argument(aArguments, "name");
   aRequest.mType      = Argument(aArguments, "type");
   aRequest.mFile      = Argument(aArguments, "file");
   aRequest.mLatitude  = Argument(aArguments, "latitude");
   aRequest.mLongitude = Argument(aArguments, "longitude");
   aRequest.mSide      = Argument(aArguments, "side");
   aRequest.mIcon      = Argument(aArguments, "icon");
   aRequest.mAltitude  = Argument(aArguments, "altitude");
   aRequest.mHeading   = Argument(aArguments, "heading");
   aRequest.mPitch     = Argument(aArguments, "pitch");
   aRequest.mRoll      = Argument(aArguments, "roll");

   if (aRequest.mName.find_first_of(" \t\r\n/#") != std::string::npos)
   {
      aError = "Platform name must not contain spaces, '/', or '#'.";
      return false;
   }

   if (aRequest.mLatitude.empty() != aRequest.mLongitude.empty())
   {
      aError = "Provide both 'latitude' and 'longitude', or neither.";
      return false;
   }

   aRequest.mLatitude  = WithDefaultHemisphere(aRequest.mLatitude, 'n');
   aRequest.mLongitude = WithDefaultHemisphere(aRequest.mLongitude, 'e');
   return true;
}

std::string AddPlatformTool::AttributeIndent()
{
   unsigned int              tabSpace = 3;
   wizard::EditorPrefObject* pref     = wkfEnv.GetPreferenceObject<wizard::EditorPrefObject>();
   if (pref)
   {
      tabSpace = pref->TabSpace();
   }
   return std::string(tabSpace, ' ');
}

void AddPlatformTool::AppendAttribute(std::string&       aPlatformDef,
                                      const std::string& aIndent,
                                      const char*        aKeyword,
                                      const std::string& aValue)
{
   if (!aValue.empty())
   {
      aPlatformDef += aIndent + aKeyword + " " + aValue + "\n";
   }
}
} // namespace McpServer
