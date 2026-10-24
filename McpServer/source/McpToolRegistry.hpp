// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLREGISTRY_HPP
#define MCPTOOLREGISTRY_HPP

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// 只前置声明，GUI 侧编译单元不会被间接引入 cxxmcp 头文件
namespace mcp::server
{
class ServerBuilder;
}

namespace McpServer
{
//! 工具执行结果；mSuccess 为 false 时 mMessage 说明失败原因
struct ToolTextResult
{
   bool        mSuccess = false;
   std::string mMessage;
};

//! 工具参数集合；当前所有工具参数均为字符串，GUI 层因此无需依赖 cxxmcp 的 Json 类型
using ToolArguments = std::map<std::string, std::string>;

//! 把一次工具调用转交给 GUI 线程执行并等待结果，由服务线程调用
using GuiToolExecutor =
   std::function<ToolTextResult(const std::string& aToolName, const ToolArguments& aArguments)>;

//! 工具参数声明：由具体工具提供，注册表据此生成输入模式并统一解析调用参数
struct ToolParameter
{
   std::string mName;
   std::string mDescription;
   bool        mRequired = false;
   //! 为 true 时保留参数值的首尾空白（用于文件内容等需要保真的文本）
   bool mPreserveWhitespace = false;
};

//! @brief MCP 工具基类：子类只描述自身元数据与 GUI 线程内的执行逻辑。
//! @note Execute() 必须在 GUI 线程中调用，由 ToolRegistry 通过 GuiToolExecutor 保证。
class McpTool
{
public:
   virtual ~McpTool() = default;

   //! 协议中 tools/call 使用的工具名
   virtual std::string Name() const = 0;

   //! 面向客户端的展示标题
   virtual std::string Title() const = 0;

   //! 面向客户端的工具说明
   virtual std::string Description() const = 0;

   //! 参数表；默认无参数
   virtual std::vector<ToolParameter> Parameters() const
   {
      return std::vector<ToolParameter>();
   }

   //! 在 GUI 线程中执行工具动作
   virtual ToolTextResult Execute(const ToolArguments& aArguments) const = 0;
};

//! @brief 工具注册表：持有全部具体工具对象，并把它们铺进 MCP 服务。
class ToolRegistry
{
public:
   //! 登记一个工具，注册表取得其所有权
   void Register(std::unique_ptr<McpTool> aTool);

   //! 按工具名查找；未登记时返回 nullptr
   const McpTool* Find(const std::string& aName) const;

   //! 把已登记的工具一次性铺进 MCP 服务构建器
   void ApplyTo(mcp::server::ServerBuilder& aBuilder, const GuiToolExecutor& aExecutor) const;

private:
   std::vector<std::unique_ptr<McpTool>> mTools;
};
} // namespace McpServer

#endif // MCPTOOLREGISTRY_HPP
