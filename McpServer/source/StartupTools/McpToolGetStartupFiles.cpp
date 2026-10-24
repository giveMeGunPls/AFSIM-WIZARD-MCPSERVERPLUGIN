// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolGetStartupFiles.hpp"

#include <string>

#include "McpFileToolSupport.hpp"
#include "McpStartupToolSupport.hpp"
#include "Project.hpp"

namespace McpServer
{
std::string GetStartupFilesTool::Name() const
{
   return "get_startup_files";
}

std::string GetStartupFilesTool::Title() const
{
   return "获取启动文件列表";
}

std::string GetStartupFilesTool::Description() const
{
   return "返回当前项目的启动文件列表。列表按执行顺序排列，第 1 项为主启动文件；"
          "同时给出相对项目目录的路径、绝对路径以及文件是否存在。本工具不修改任何状态。";
}

std::vector<ToolParameter> GetStartupFilesTool::Parameters() const
{
   return {};
}

ToolTextResult GetStartupFilesTool::Execute(const ToolArguments&) const
{
   ToolTextResult result;
   std::string    error;

   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   const std::vector<UtPath> files = wizard::Project::Instance()->GetStartupFiles();

   result.mSuccess = true;
   result.mMessage = "Startup files (" + std::to_string(files.size()) + "):\n";
   if (files.empty())
   {
      result.mMessage += "  The project has no startup files.\n";
   }
   else
   {
      result.mMessage += StartupTools::DescribeStartupFiles(files);
   }
   return result;
}
} // namespace McpServer
