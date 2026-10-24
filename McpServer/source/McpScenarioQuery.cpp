// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpScenarioQuery.hpp"

#include <map>
#include <string>
#include <vector>

#include "ParseResults.hpp"
#include "Project.hpp"
#include "TextSource.hpp"
#include "UtPath.hpp"
#include "UtTextDocument.hpp"
#include "WsfPM_Root.hpp"
#include "WsfPProxy.hpp"
#include "WsfPProxyIndex.hpp"
#include "WsfPProxyObjectMap.hpp"
#include "WsfParseNode.hpp"

namespace
{
using FileHitMap = std::map<std::string, std::size_t>;

//! 相对项目目录的路径；文档不在项目中时返回空串
std::string RelativeDocPath(const UtTextDocument* aDoc)
{
   std::string path;
   wizard::Project* project = wizard::Project::Instance();
   if (aDoc && project)
   {
      path = project->ProjectDirectory().GetRelativePath(aDoc->GetFilePath()).GetSystemPath();
   }
   return path;
}

//! 按源文件统计平台定义数量，用于挑选默认的目标脚本文件
void CountPlatformsByFile(FileHitMap& aFileHitMap)
{
   wizard::Project* project = wizard::Project::Instance();
   if (!project)
   {
      return;
   }

   const wizard::ParseResults* parseResults = project->GetParseResults();
   if (parseResults)
   {
      for (const wizard::ParseIncludes::value_type& entry : parseResults->mFirstIncludes)
      {
         const std::string relativePath = RelativeDocPath(entry.first);
         if (!relativePath.empty())
         {
            aFileHitMap[relativePath] = 0;
         }
      }
   }

   WsfPProxy* proxy = project->Proxy();
   if (!proxy)
   {
      return;
   }

   std::vector<WsfPProxyNode> platforms;
   WsfPM_Root(proxy).platforms().GetAllValues(platforms);

   for (const WsfPProxyNode& platformNode : platforms)
   {
      const WsfPProxyIndexNode* indexNode = platformNode.FindIndexNode();
      if (!indexNode)
      {
         continue;
      }

      for (const WsfPProxyIndexEntry& indexEntry : indexNode->mEntries)
      {
         if (indexEntry.mEntryType != WsfPProxyIndex::cADDED)
         {
            continue;
         }

         const WsfParseNode* parseNode = indexEntry.mNodePtr->PreviousLeaf();
         if (!parseNode)
         {
            continue;
         }

         const std::string relativePath = RelativeDocPath(parseNode->mValue.mSource);
         FileHitMap::iterator iter      = aFileHitMap.find(relativePath);
         if (iter != aFileHitMap.end())
         {
            ++iter->second;
         }
         break;
      }
   }
}
} // namespace

namespace McpServer
{
std::string ScenarioQuery::DefaultTargetFile()
{
   FileHitMap fileHitMap;
   CountPlatformsByFile(fileHitMap);

   // std::map 按路径升序遍历，只有数量严格更多时才替换，数量相同自然保留字母序在前的文件
   FileHitMap::const_iterator best = fileHitMap.begin();
   for (FileHitMap::const_iterator iter = fileHitMap.begin(); iter != fileHitMap.end(); ++iter)
   {
      if (iter->second > best->second)
      {
         best = iter;
      }
   }

   return (best == fileHitMap.end()) ? std::string() : best->first;
}
} // namespace McpServer
