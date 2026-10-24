// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolGetCurrentStartupFile.hpp"

#include <string>
#include <vector>

#include "McpFileToolSupport.hpp"
#include "McpStartupToolSupport.hpp"
#include "Project.hpp"

namespace McpServer
{
std::string GetCurrentStartupFileTool::Name() const
{
   return "get_current_startup_file";
}

std::string GetCurrentStartupFileTool::Title() const
{
   return "获取当前启动文件";
}

std::string GetCurrentStartupFileTool::Description() const
{
   return "返回当前项目的主启动文件，即启动文件列表的第 1 项。项目没有启动文件时返回失败。";
}

std::vector<ToolParameter> GetCurrentStartupFileTool::Parameters() const
{
   return {};
}

ToolTextResult GetCurrentStartupFileTool::Execute(const ToolArguments&) const
{
   ToolTextResult result;
   std::string    error;

   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   const std::vector<UtPath> files = wizard::Project::Instance()->GetStartupFiles();
   if (files.empty())
   {
      result.mMessage = "The project has no startup files.";
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Current startup file (1 of " + std::to_string(files.size()) + "):\n" +
                     StartupTools::DescribeStartupFiles(std::vector<UtPath>{files.front()});
   return result;
}
} // namespace McpServer
