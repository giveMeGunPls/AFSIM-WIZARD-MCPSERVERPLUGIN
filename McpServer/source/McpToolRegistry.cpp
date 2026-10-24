// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolRegistry.hpp"

#include <string>
#include <utility>
#include <vector>

#include "cxxmcp/core/result.hpp"
#include "cxxmcp/protocol.hpp"
#include "cxxmcp/server.hpp"
#include "cxxmcp/server/context.hpp"
#include "cxxmcp/server/handler_types.hpp"

namespace
{
std::string Trimmed(const std::string& aValue)
{
   const std::string whitespace = " \t\r\n";
   const size_t      first      = aValue.find_first_not_of(whitespace);
   if (first == std::string::npos)
   {
      return std::string();
   }
   const size_t last = aValue.find_last_not_of(whitespace);
   return aValue.substr(first, last - first + 1);
}

//! 按工具声明的参数表生成输入模式
mcp::protocol::Json BuildInputSchema(const std::vector<McpServer::ToolParameter>& aParameters)
{
   mcp::protocol::Json properties = mcp::protocol::Json::object();
   mcp::protocol::Json required   = mcp::protocol::Json::array();
   for (const McpServer::ToolParameter& parameter : aParameters)
   {
      mcp::protocol::Json property = mcp::protocol::Json::object();
      property["type"]             = "string";
      property["description"]      = parameter.mDescription;
      properties[parameter.mName]  = std::move(property);
      if (parameter.mRequired)
      {
         required.push_back(parameter.mName);
      }
   }

   mcp::protocol::Json schema = mcp::protocol::Json::object();
   schema["type"]             = "object";
   schema["properties"]       = std::move(properties);
   if (!required.empty())
   {
      schema["required"] = std::move(required);
   }
   schema["additionalProperties"] = false;
   return schema;
}

bool IsDeclaredParameter(const std::vector<McpServer::ToolParameter>& aParameters, const std::string& aName)
{
   for (const McpServer::ToolParameter& parameter : aParameters)
   {
      if (parameter.mName == aName)
      {
         return true;
      }
   }
   return false;
}

//! 按参数表把 JSON 参数转成字符串集合，未声明的参数一律拒绝
bool ExtractArguments(const mcp::protocol::Json&                   aArguments,
                      const std::vector<McpServer::ToolParameter>& aParameters,
                      McpServer::ToolArguments&                    aTarget,
                      std::string&                                 aError)
{
   if (!aArguments.is_object())
   {
      aError = "Arguments must be a JSON object.";
      return false;
   }

   for (const McpServer::ToolParameter& parameter : aParameters)
   {
      const mcp::protocol::Json::const_iterator iter = aArguments.find(parameter.mName);
      if ((iter == aArguments.end()) || iter->is_null())
      {
         if (parameter.mRequired)
         {
            aError = "Argument '" + parameter.mName + "' is required.";
            return false;
         }
         continue;
      }
      if (!iter->is_string())
      {
         aError = "Argument '" + parameter.mName + "' must be a string.";
         return false;
      }

      const std::string value =
         parameter.mPreserveWhitespace ? iter->get<std::string>() : Trimmed(iter->get<std::string>());
      if (parameter.mRequired && value.empty())
      {
         aError = "Argument '" + parameter.mName + "' is required.";
         return false;
      }
      aTarget[parameter.mName] = value;
   }

   for (mcp::protocol::Json::const_iterator iter = aArguments.begin(); iter != aArguments.end(); ++iter)
   {
      if (!IsDeclaredParameter(aParameters, iter.key()))
      {
         aError = "Unknown argument '" + iter.key() + "'.";
         return false;
      }
   }
   return true;
}

//! 生成 MCP 处理器：解析参数后交给 GUI 线程执行工具对象，并把结果映射成工具返回值
mcp::server::ToolHandler MakeToolHandler(std::string                           aToolName,
                                         std::vector<McpServer::ToolParameter> aParameters,
                                         McpServer::GuiToolExecutor            aExecutor)
{
   return [name = std::move(aToolName), parameters = std::move(aParameters), executor = std::move(aExecutor)](
             const mcp::server::ToolContext& aContext) -> mcp::core::Result<mcp::protocol::ToolResult>
   {
      McpServer::ToolArguments arguments;
      std::string              error;
      if (!ExtractArguments(aContext.arguments, parameters, arguments, error))
      {
         return mcp::protocol::ToolResult::error_text(std::move(error));
      }
      if (!executor)
      {
         return mcp::protocol::ToolResult::error_text(name + " is not connected to the Wizard user interface.");
      }

      const McpServer::ToolTextResult result = executor(name, arguments);
      if (result.mMessage.empty())
      {
         return mcp::protocol::ToolResult::error_text(name + " timed out waiting for the Wizard user interface.");
      }
      if (!result.mSuccess)
      {
         return mcp::protocol::ToolResult::error_text(result.mMessage);
      }
      return mcp::protocol::ToolResult::text(result.mMessage);
   };
}
} // namespace

namespace McpServer
{
void ToolRegistry::Register(std::unique_ptr<McpTool> aTool)
{
   if (aTool)
   {
      mTools.push_back(std::move(aTool));
   }
}

const McpTool* ToolRegistry::Find(const std::string& aName) const
{
   for (const std::unique_ptr<McpTool>& tool : mTools)
   {
      if (tool->Name() == aName)
      {
         return tool.get();
      }
   }
   return nullptr;
}

void ToolRegistry::ApplyTo(mcp::server::ServerBuilder& aBuilder, const GuiToolExecutor& aExecutor) const
{
   for (const std::unique_ptr<McpTool>& tool : mTools)
   {
      const std::string          name        = tool->Name();
      const std::string          title       = tool->Title();
      const std::string          description = tool->Description();
      std::vector<ToolParameter> parameters  = tool->Parameters();

      mcp::protocol::ToolDefinition definition =
         mcp::protocol::tool_definition(name)
            .title(title)
            .description(description)
            .input_schema(BuildInputSchema(parameters))
            .build();

      aBuilder.add_tool(std::move(definition), MakeToolHandler(name, std::move(parameters), aExecutor));
   }
}
} // namespace McpServer
