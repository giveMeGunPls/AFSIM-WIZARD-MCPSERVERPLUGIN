// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolDeletePlatform.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "FileTools/McpFileToolSupport.hpp"
#include "McpScenarioEditSupport.hpp"
#include "ProjectWorkspace.hpp"
#include "TextSource.hpp"
#include "UtTextDocument.hpp"
#include "WsfParseNode.hpp"
#include "WsfPProxyIndex.hpp"

namespace
{
//! 待删除的脚本区间；同一文件内按起点从后往前删，避免偏移失效
struct DeleteBlock
{
   std::string mPath;
   size_t      mBegin = 0;
   size_t      mEnd   = 0;
};

bool LessForDelete(const DeleteBlock& aLhs, const DeleteBlock& aRhs)
{
   if (aLhs.mPath != aRhs.mPath)
   {
      return aLhs.mPath < aRhs.mPath;
   }
   return aLhs.mBegin > aRhs.mBegin;
}
} // namespace

namespace McpServer
{
std::string DeletePlatformTool::Name() const
{
   return "delete_platform";
}

std::string DeletePlatformTool::Title() const
{
   return "删除平台";
}

std::string DeletePlatformTool::Description() const
{
   return "从 AFSIM Wizard 当前打开的想定中删除定义该平台的所有脚本块"
          "（基础定义以及所有 'edit platform' 块）。";
}

std::vector<ToolParameter> DeletePlatformTool::Parameters() const
{
   return {
      {"name", "要删除的平台名称。", true},
   };
}

ToolTextResult DeletePlatformTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string name = ScenarioEdit::Argument(aArguments, "name");

   std::string   error;
   WsfPProxyNode node;
   if (!ScenarioEdit::FindPlatform(name, node, error))
   {
      result.mMessage = error;
      return result;
   }

   // 与 wizard 的删除逻辑一致：只有一个索引项时删平台定义，
   // 存在 'edit platform' 块时删各编辑块所在的节点子树
   std::vector<UtTextDocumentRange> ranges;
   const WsfPProxyIndexNode*        indexNode = node.FindIndexNode();
   if (indexNode && !indexNode->mEntries.empty())
   {
      if (indexNode->mEntries.size() == 1)
      {
         ranges.push_back(node.GetSourceDocumentRange());
      }
      else
      {
         for (const WsfPProxyIndexEntry& entry : indexNode->mEntries)
         {
            const WsfParseNode* parseNode = entry.mNodePtr;
            if (parseNode && parseNode->GetParent())
            {
               ranges.push_back(parseNode->GetParent()->SubtreeRange());
            }
         }
      }
   }
   if (ranges.empty())
   {
      ranges.push_back(node.GetSourceDocumentRange());
   }

   std::vector<DeleteBlock> blocks;
   for (const UtTextDocumentRange& range : ranges)
   {
      if (!range.Valid() || (range.GetEnd() == ut::npos) || (range.GetEnd() < range.GetBegin()))
      {
         continue;
      }

      const DeleteBlock candidate{range.mSource->GetFilePath().GetSystemPath(),
                                  range.GetBegin(),
                                  range.GetEnd()};
      const bool duplicate = std::any_of(blocks.begin(), blocks.end(), [&](const DeleteBlock& aBlock)
                                         {
                                            return (aBlock.mPath == candidate.mPath) &&
                                                   (aBlock.mBegin == candidate.mBegin) &&
                                                   (aBlock.mEnd == candidate.mEnd);
                                         });
      if (!duplicate)
      {
         blocks.push_back(candidate);
      }
   }

   if (blocks.empty())
   {
      result.mMessage = "No deletable script range was found for platform '" + name + "'.";
      return result;
   }

   std::sort(blocks.begin(), blocks.end(), LessForDelete);

   wizard::ProjectWorkspace* workspace = wizard::ProjectWorkspace::Instance();
   for (const DeleteBlock& block : blocks)
   {
      wizard::TextSource* source =
         workspace ? workspace->FindSource(block.mPath, false, false) : nullptr;
      if (!source)
      {
         result.mMessage = "Unable to locate the script file: " + block.mPath;
         return result;
      }
      if (!FileTools::EnsureLoaded(source, error))
      {
         result.mMessage = error;
         return result;
      }

      source->Replace(UtTextRange(block.mBegin, block.mEnd), std::string());
   }

   result.mSuccess = true;
   result.mMessage = "Deleted platform '" + name + "' (" + std::to_string(blocks.size()) +
                     " script block(s) removed).";
   return result;
}
} // namespace McpServer
