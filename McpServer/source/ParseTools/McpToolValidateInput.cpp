// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolValidateInput.hpp"

#include <memory>
#include <string>
#include <vector>

#include "Project.hpp"
#include "TextSourceCache.hpp"
#include "UtPath.hpp"
#include "UtTextDocument.hpp"
#include "WsfExe.hpp"
#include "WsfParseDefinitions.hpp"
#include "WsfParseSourceProvider.hpp"
#include "WsfParser.hpp"

namespace
{
const std::string cINLINE_FILE_NAME = "mcp_inline_input.wsf";

//! 把内存中的一段文本当作虚拟脚本文件提供给解析器，其余路径仍交给项目自身的源缓存
class InlineSourceProvider : public WsfParseSourceProvider
{
public:
   InlineSourceProvider(UtTextDocument* aDocumentPtr, WsfParseSourceProvider* aFallbackPtr)
      : mDocumentPtr(aDocumentPtr)
      , mFallbackPtr(aFallbackPtr)
   {
   }

   UtTextDocument* FindSource(const UtPath& aPath, bool aReadAccess) override
   {
      if (!aReadAccess)
      {
         return nullptr;
      }
      if ((aPath.GetFileName() == cINLINE_FILE_NAME) || (aPath.GetSystemPath() == cINLINE_FILE_NAME))
      {
         return mDocumentPtr;
      }
      return mFallbackPtr ? mFallbackPtr->FindSource(aPath, aReadAccess) : nullptr;
   }

private:
   UtTextDocument*         mDocumentPtr;
   WsfParseSourceProvider* mFallbackPtr;
};

//! 由区间换算出“第 n 行, 第 m 列”；区间不在内联文档上时返回空串
std::string RangeLabel(UtTextDocument& aDocument, const UtTextDocumentRange& aRange)
{
   if (aRange.mSource != &aDocument)
   {
      return std::string();
   }
   size_t line   = 0;
   size_t column = 0;
   aDocument.PositionToLineColumn(aRange.GetBegin(), line, column);
   return "第 " + std::to_string(line + 1) + " 行, 第 " + std::to_string(column + 1) + " 列";
}

//! 取出区间原文并压成一行，方便直接放进消息里
std::string RangeText(const UtTextDocumentRange& aRange)
{
   if (!aRange.Valid())
   {
      return std::string();
   }
   std::string text = aRange.Text();
   for (char& character : text)
   {
      if ((character == '\n') || (character == '\r') || (character == '\t'))
      {
         character = ' ';
      }
   }
   return text;
}
} // namespace

namespace McpServer
{
std::string ValidateInputTool::Name() const
{
   return "validate_input";
}

std::string ValidateInputTool::Title() const
{
   return "校验输入块";
}

std::string ValidateInputTool::Description() const
{
   return "按当前项目的语法解析一段 AFSIM 输入（平台、区域、航线、定义等），"
          "并报告语法无法接受的每条命令或标记。文本只在内存中解析，不会写入脚本文件。"
          "脚本正文（script body）不参与检查。";
}

std::vector<ToolParameter> ValidateInputTool::Parameters() const
{
   return {
      {"input", "要校验的 AFSIM 输入文本。", true, true},
   };
}

ToolTextResult ValidateInputTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;

   const auto inputIt = aArguments.find("input");
   if ((inputIt == aArguments.end()) || inputIt->second.empty())
   {
      result.mMessage = "Missing 'input' argument.";
      return result;
   }
   const std::string& text = inputIt->second;

   wizard::Project* project = wizard::Project::Instance();
   if (!project)
   {
      result.mMessage = "No project is open.";
      return result;
   }

   // 后台解析线程与当前校验会共用同一份语法定义，先等既有解析结束，避免并发访问规则树
   project->WaitForAllParsing();

   wizard::WsfExe* exePtr = project->GetExe();
   if (!exePtr)
   {
      result.mMessage = "The project has no executable loaded, so no grammar is available.";
      return result;
   }

   std::shared_ptr<WsfParseDefinitions> definitions = exePtr->GetParseDefinitions();
   if (!definitions || (definitions->GetRoot() == nullptr))
   {
      result.mMessage = "The project grammar is not available.";
      return result;
   }

   UtTextDocument document;
   document.Insert(0, text);

   InlineSourceProvider provider(&document, project->SourceProvider());
   // WsfParser 会接管构造函数传入的定义并在析构时 delete，不能直接传入共享指针的裸指针；
   // 这里让它先自建一份空定义，再用 SwapDefinitions 借入项目定义，解析结束后换回
   WsfParser            parser(&provider, project->WorkingDirectory());
   WsfParseDefinitions* exchange = definitions.get();
   parser.SwapDefinitions(exchange); // exchange 此时指向解析器自建的空定义
   parser.ParseFiles({UtPath(cINLINE_FILE_NAME)}, 50);
   parser.SwapDefinitions(exchange); // 归还项目定义，解析器重新持有空定义并在析构时自行释放

   std::vector<std::string> messages;
   for (const WsfParseError* errorPtr : parser.GetErrors())
   {
      if (!errorPtr)
      {
         continue;
      }
      const std::string label = RangeLabel(document, errorPtr->mRange);
      if (label.empty())
      {
         continue; // 来自被包含文件（或已被释放的源）的错误不属于本次校验
      }
      const std::string snippet = RangeText(errorPtr->mRange);
      messages.push_back(label + ": 无法匹配的输入" + (snippet.empty() ? std::string() : " '" + snippet + "'"));
      if (messages.size() >= 100)
      {
         break;
      }
   }

   if (messages.empty())
   {
      result.mSuccess = true;
      result.mMessage = "Input syntax check passed.";
      return result;
   }

   result.mMessage = "Input syntax check found " + std::to_string(messages.size()) + " error(s):\n";
   for (size_t i = 0; i < messages.size(); ++i)
   {
      result.mMessage += (i == 0 ? "" : "\n") + messages[i];
   }
   return result;
}
} // namespace McpServer
