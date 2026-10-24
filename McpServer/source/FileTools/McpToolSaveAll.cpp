// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolSaveAll.hpp"

#include <map>
#include <string>

#include "McpFileToolSupport.hpp"
#include "Project.hpp"
#include "ProjectWorkspace.hpp"
#include "TextSource.hpp"
#include "TextSourceCache.hpp"

namespace McpServer
{
namespace
{
//! 统计项目中仍有未保存修改的文件数
size_t CountModifiedSources(wizard::Project* aProjectPtr)
{
   size_t count = 0;
   for (const auto& entry : aProjectPtr->GetSourceCache().GetSources())
   {
      if ((entry.second != nullptr) && entry.second->IsModified())
      {
         ++count;
      }
   }
   return count;
}
} // namespace

std::string SaveAllTool::Name() const
{
   return "save_all";
}

std::string SaveAllTool::Title() const
{
   return "Save All";
}

std::string SaveAllTool::Description() const
{
   return "保存当前项目中所有已修改的文件，等效于 File > Save All。";
}

std::vector<ToolParameter> SaveAllTool::Parameters() const
{
   return {};
}

ToolTextResult SaveAllTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   wizard::Project* workspaceProject = wizard::Project::Instance();
   const size_t     modifiedCount    = CountModifiedSources(workspaceProject);
   wizard::ProjectWorkspace::Instance()->SaveAll();

   const size_t remainingCount = CountModifiedSources(workspaceProject);
   if (remainingCount > 0)
   {
      result.mMessage = "Saved " + std::to_string(modifiedCount - remainingCount) + " file(s), but " +
                        std::to_string(remainingCount) + " file(s) could not be saved.";
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Saved " + std::to_string(modifiedCount) + " modified file(s).";
   return result;
}
} // namespace McpServer
