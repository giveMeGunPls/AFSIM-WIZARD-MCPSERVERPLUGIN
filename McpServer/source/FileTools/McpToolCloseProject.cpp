// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolCloseProject.hpp"

#include <string>

#include "McpFileToolSupport.hpp"
#include "Project.hpp"
#include "ProjectWorkspace.hpp"

namespace McpServer
{
std::string CloseProjectTool::Name() const
{
   return "close_project";
}

std::string CloseProjectTool::Title() const
{
   return "Close Project";
}

std::string CloseProjectTool::Description() const
{
   return "关闭当前打开的项目，等效于 File > Close Project。未保存的修改会被丢弃，请先使用 save_all。";
}

std::vector<ToolParameter> CloseProjectTool::Parameters() const
{
   return {};
}

ToolTextResult CloseProjectTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   wizard::ProjectWorkspace* workspace = wizard::ProjectWorkspace::Instance();
   const QString             projectName = wizard::Project::Instance()->Name();
   if (!workspace->RequestClose(true))
   {
      result.mMessage = "Could not close the project.";
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Closed project '" + projectName.toStdString() + "'.";
   return result;
}
} // namespace McpServer
