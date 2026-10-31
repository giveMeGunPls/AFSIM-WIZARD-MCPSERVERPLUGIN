# McpServer 插件

将 AFSIM Wizard 的场景编辑能力以 [MCP](https://modelcontextprotocol.io) 工具的形式，
通过 Streamable HTTP 暴露给外部 MCP 客户端（Claude Code、各类 IDE Agent 等）。

插件在 Wizard 中表现为右侧的「MCP 服务」面板：设置监听端口后点击「启动服务」，
Wizard 便会在 `127.0.0.1` 上提供 MCP 端点。所有工具动作最终都在 GUI 线程中执行，
因此工具读写的就是当前打开的项目，与手工操作完全等价。

- 端点：`http://127.0.0.1:<端口>/mcp`（默认端口 8765，仅监听回环地址）
- 协议版本：`2025-03-26`
- 请求头需包含 `Accept: application/json, text/event-stream`
- 会话通过响应头 `Mcp-Session-Id` 维持，结束时对该端点发送 `DELETE`

## 1. 构建依赖

本插件依赖两个需要自行准备的第三方 SDK，CMake 无法自动下载。

### 1.1 cxxmcp（必需）

cxxmcp 提供 MCP 服务端实现，本仓库使用其预编译静态库。

- 项目主页：<https://github.com/caomengxuan666/cxxmcp>
- 源码下载：<https://github.com/caomengxuan666/cxxmcp/releases>（也可直接
  `git clone https://github.com/caomengxuan666/cxxmcp.git`）
- 本仓库验证过的版本：**1.3.0**

从源码构建（要求 CMake 3.23+、支持 C++17 的编译器，构建时还需要 OpenSSL）：

```
git clone https://github.com/caomengxuan666/cxxmcp.git
cd cxxmcp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build --config Release --prefix E:/cxxmcp_sdk/Release
```

安装完成后按下面的目录结构放置（`--prefix` 指向上面的 `Release` 目录即可）：

```
<cxxmcp 根目录>/
└── Release/
    ├── include/          # cxxmcp 头文件
    └── lib/
        ├── mcp_server.lib
        └── mcp_protocol.lib
```

然后在配置阶段指定根目录（默认值为 `E:/cxxmcp_sdk`）：

```
-DCXXMCP_SDK_ROOT=<cxxmcp 根目录>
```

说明：

- cxxmcp 头文件要求 **C++17**，`CMakeLists.txt` 已对目标单独开启 `cxx_std_17`。
- 本仓库只链接 `Release` 目录下的头文件与 `/MD` 静态库，Debug 构建同样使用这一份。
- 若实际路径与默认值不同而缓存中已存在旧值，请用 `cmake-gui` 或删除构建目录的
  `CMakeCache.txt` 后重新配置，`-D` 参数不会覆盖已缓存的变量。

### 1.2 OpenSSL（必需）

`mcp_server.lib` 内置的 httplib 带 SSL 支持，链接时需要有 OpenSSL 符号。这里使用
**静态库（`/MD` 版）**，避免运行时额外部署 `libcrypto-3-x64.dll` / `libssl-3-x64.dll`。

- 官方源码：<https://www.openssl.org/source/>
- Windows 预编译包：<https://slproweb.com/products/Win32OpenSSL.html>（选择 3.x 的
  Win64 完整版安装包，安装后即包含 `lib/VC/static` 下的 `/MD` 静态库）
- 默认根目录：`D:/OpenSSL-Win64`，可用 `-DOPENSSL_SDK_ROOT=<OpenSSL 根目录>` 覆盖
- 需要 `include/`、`lib/VC/static/libssl64MD.lib`、`lib/VC/static/libcrypto64MD.lib`
- 若使用自编译版本，需保证与 Visual Studio 版本、`/MD` 运行时一致

> 因为链接的是 `/MD` 静态库，本目标必须使用 `/MD` 运行时，不能与 `/MT` 混用，
> 否则会出现 CRT 符号冲突。

### 1.3 配置示例

```
cmake -S <源码目录> -B <构建目录> ^
      -DCXXMCP_SDK_ROOT=E:/cxxmcp_sdk ^
      -DOPENSSL_SDK_ROOT=D:/OpenSSL-Win64
```

> **目录位置提示**：本插件必须把整个 `McpServer` 文件夹原样放到
> `src\wizard\plugins\` 下（即 `src\wizard\plugins\McpServer\`），
> 保持 `wizard_plugin.cmake`、`source\CMakeLists.txt` 与源码的相对结构不变，
> 否则 Wizard 的构建系统无法发现该插件。

插件由 `wizard_plugin.cmake` 声明（`WIZARD_PLUGIN_NAME=WizMcpServer`），
Wizard 的构建系统会自动纳入，无需在主 CMake 中手动 `add_subdirectory`。

`source/CMakeLists.txt` 会递归收集各工具子目录（`FileTools`、`PlatformTools` 等）下的
所有 `.cpp/.hpp`，因此新增工具文件不需要改构建脚本。

平台依赖：Windows 专用（链接 `ws2_32`、`crypt32`、`advapi32`、`user32`）。

## 2. 现有工具

工具按领域分目录存放：`source/FileTools`（文件与项目操作）、`source/PlatformTools`
（场景平台操作）、`source/ParseTools`（解析校验）、`source/StartupTools`（启动文件）、
`source/ConfigTools`（全局仿真配置）。目前共 **41 个工具**（文件 10、平台 20、解析 1、
启动文件 6、全局配置 4），逐项参数与约束见 [MCP工具清单.md](MCP工具清单.md)。
所有文件路径参数都必须在**当前项目目录内**，绝对路径或 `..` 越界会被拒绝。

### 2.1 文件类（`FileTools`）

| 工具名 | 作用 | 参数 |
| --- | --- | --- |
| `new_file` | 新建文本文件并打开到编辑器 | `file`（必填）、`content`（可选，按原始字节写入，保留首尾空白） |
| `open_file` | 打开项目中已存在的文件 | `file`（必填） |
| `save_file` | 保存文件；未修改时不重写磁盘 | `file`（可选，缺省为当前活动文件） |
| `save_file_as` | 另存为；未修改时按原始字节复制 | `file`（可选）、`newFile`（必填）、`overwrite`（可选） |
| `save_all` | 保存项目中全部已修改文件 | 无 |
| `print_file` | 打印文件；指定 `output` 时输出 PDF | `file`（可选）、`output`（可选，缺省走系统打印机） |
| `new_project` | 新建项目，须为绝对路径且以 `.afproj` 结尾 | `file`（必填） |
| `close_project` | 关闭当前项目 | 无 |
| `save_project` | 保存项目文件 | `file`（可选；项目未绑定时必填） |
| `export_project` | 导出项目包含的文件 | `directory`（必填，须已存在）、`overwrite`（可选） |

### 2.2 平台类（`PlatformTools`）

| 工具名 | 作用 | 参数 |
| --- | --- | --- |
| `list_platform_types` | 列出可用平台类型、已有平台名与默认目标脚本 | 无 |
| `add_platform` | 向目标脚本追加平台定义 | `name`（必填）、`type`（必填）、`file`、`latitude`、`longitude`、`side`、`icon`、`altitude`、`heading`、`pitch`、`roll` |
| `delete_platform` | 删除平台的全部脚本定义块 | `name`（必填） |
| `clone_platform` | 复制平台定义并改名后追加到同一脚本 | `name`（必填）、`new_name` |
| `find_platform` | 返回平台所在文件、行号、类型与定义原文 | `name`（必填） |
| `set_platform_attributes` | 就地修改平台顶层属性行 | `name`（必填）、`side`、`icon`、`latitude`、`longitude`、`altitude`、`heading`、`pitch`、`roll` |

平台资源（数据源与 PlatformData 插件的 Side / Icon 下拉框一致，不依赖已打开的项目）：

| 工具名 | 作用 | 参数 |
| --- | --- | --- |
| `list_sides` | 列出可选的阵营（side）名称，即颜色表中的颜色名 | `side`（可选，只查询单个阵营） |
| `list_icons` | 按分类列出模型数据库中的图标（icon）名称 | `category`（可选，只查询单个分类；未分类的图标归入 `uncategorized`） |

平台部件：

| 工具名 | 作用 | 参数 |
| --- | --- | --- |
| `list_platform_part_types` | 列出平台部件可用类型（脚本定义与内置，按类别分组） | `kind`（可选：sensor/weapon/comm/processor/mover/fuel） |
| `list_platform_parts` | 列出平台部件（种类、名称、类型、行号） | `platform`（必填） |
| `add_platform_part` | 向平台插入部件块 | `platform`（必填）、`kind`（必填）、`type`（必填）、`name` |
| `remove_platform_part` | 删除平台部件块 | `platform`（必填）、`name`、`kind` |

航线与航路点：

| 工具名 | 作用 | 参数 |
| --- | --- | --- |
| `add_route` | 为平台添加航线块，或追加全局航线 | `platform`、`scope`、`route`、`file`、`latitude`、`longitude`、`altitude`、`speed` |
| `add_waypoint` | 在航线末尾追加航路点 | `latitude`（必填）、`longitude`（必填）、`platform`、`scope`、`route`、`altitude`、`speed`、`label` |
| `update_waypoint` | 按序号修改航路点 | `index`（必填）、`platform`、`scope`、`route`、`latitude`、`longitude`、`altitude`、`speed`、`label` |
| `remove_waypoint` | 按序号删除航路点 | `index`（必填）、`platform`、`scope`、`route` |
| `remove_route` | 删除平台航线块或全局航线 | `platform`、`scope`、`route` |

区域：

| 工具名 | 作用 | 参数 |
| --- | --- | --- |
| `add_zone` | 向目标脚本追加区域定义 | `name`（必填）、`latitude`、`longitude`（非多边形区域必填）、`type`、`file`、`heading`、`minimum_altitude`、`maximum_altitude`、`minimum_radius`、`maximum_radius`、`lateral_axis`、`longitudinal_axis`、`start_angle`、`stop_angle`、`fill_color`、`line_color`、`points`（多边形必填） |
| `find_zone` | 返回区域所在文件、行号、形状与定义原文 | `name`（必填） |
| `remove_zone` | 删除区域定义 | `name`（必填） |

这些工具都不操作 GUI 对象，只修改脚本原文（追加或在定位到的区间上就地替换），
随后由 Wizard 重新解析脚本生成场景对象。`scope` 缺省为 `platform`；
全局航线需要 `scope: "global"` 并由 `route` 指定航线名。

### 2.3 解析校验类（`ParseTools`）

| 工具名 | 作用 | 参数 |
| --- | --- | --- |
| `validate_input` | 用当前项目的语法定义独立解析一段 AFSIM 输入块，返回无法匹配的命令 | `input`（必填，保留首尾空白） |

- `validate_input` 用独立 `WsfParser` 在内存文档上解析，不读写任何文件，适合校验 `platform`、
  `zone`、`route`、类型定义等输入块。
- 它只做块语法这一层，**脚本函数体（`script ... end_script` 内部）不在此校验**：
  wizard_core 未导出脚本解析器内部类型，插件侧无法直接调用 `WsfScriptParser`。
- 不修改场景，`mSuccess` 为 `false` 表示语法未通过，错误清单在 `mMessage` 中（行列 + 出错片段）。

### 2.4 启动文件类（`StartupTools`）

| 工具名 | 作用 | 参数 |
| --- | --- | --- |
| `get_startup_files` | 列出启动文件（顺序、主文件标记、相对/绝对路径、是否存在） | 无 |
| `get_current_startup_file` | 返回主启动文件（列表第 1 项） | 无 |
| `set_current_startup_file` | 用单个文件替换启动文件列表（等效 "Set as Startup File"） | `file`（必填）、`save`（可选，缺省 true） |
| `add_startup_file` | 追加到启动文件列表末尾（等效 "Add to Startup Files"） | `file`（必填）、`save`（可选，缺省 true） |
| `remove_startup_file` | 从列表移除一项；不允许移除唯一项 | `file`（必填）、`save`（可选，缺省 true） |
| `set_startup_files` | 按顺序整体替换列表（等效 "Configure Startup Files"） | `files`（必填，每行一个）、`save`（可选，缺省 true） |

- 启动文件按顺序作为命令行参数传给仿真程序，第 1 项为主启动文件；参数路径须存在、为文本文件且位于项目目录内。
- 工具直接读写 `wizard::Project` 的启动文件列表，不接触任何 UI 或对话框；项目数据变化后由 Wizard 信号自动刷新界面。
- 修改默认会调用 `SaveProject()` 写入 `.afproj`；项目尚未绑定项目文件时仅改内存，并在返回信息中提示改用 `save_project`。
- `save=false` 时只改内存，可稍后统一调用 `save_project`。`set_startup_files` 的 `files` 用换行分隔且不允许重复路径。

### 2.5 全局配置类（`ConfigTools`）

| 工具名 | 作用 | 参数 |
| --- | --- | --- |
| `set_log_file` | 开启日志记录并设置日志文件，生成 `log_file <path>` | `path`（必填）、`file`（可选，缺省主启动文件） |
| `set_event_output` | 开启事件记录并设置记录文件，生成 `event_output ... end_event_output` 块 | `path`（必填）、`file`（可选） |
| `set_event_pipe` | 开启事件回放并设置回放文件与预设，生成 `event_pipe ... end_event_pipe` 块 | `path`（必填）、`preset`（可选，缺省 `full`）、`file`（可选） |
| `set_end_time` | 设置仿真总时长，生成 `end_time <time>` | `time`（必填，如 `1 hour`）、`file`（可选） |

- 目标脚本缺省取启动文件列表第 1 项（主启动文件）；没有启动文件时回退到平台定义最多的脚本。
- 单行命令已有同名顶层命令时就地替换；`event_output` / `event_pipe` 已有顶层块时只更新
  `file`（以及 `use_preset`）参数行，保留块内其他事件开关设置；块不存在时才追加完整块。
- 这些工具只修改脚本原文，不接触仿真程序或任何 UI；输出路径按传入内容原样写入脚本。

## 3. 运行机制

```
MCP 客户端
    │  HTTP (Streamable HTTP)
    ▼
ToolServer（服务线程）        McpToolServer.cpp
    │  参数解析 / 结果映射      McpToolRegistry.cpp
    ▼
GuiToolExecutor
    │  QMetaObject::invokeMethod (QueuedConnection)
    ▼
McpTool::Execute（GUI 线程）  FileTools/、PlatformTools/、ParseTools/
    │
    ▼
wizard::Project / ProjectWorkspace / TextSource
```

要点：

- `ToolRegistry` 持有全部工具对象，并在服务启动时一次性铺进 cxxmcp 的 `ServerBuilder`。
- 工具对象只描述元数据，真正的动作通过 `GuiToolExecutor` 投递到 GUI 线程执行，
  服务线程最多等待 5 秒；因此 `Execute()` 内不能弹出模态对话框或长时间阻塞。
- 所有参数在注册表层统一解析为字符串，未声明的参数会被直接拒绝。

## 4. 扩展新工具

以新增一个 `rename_file` 为例，共四步。

### 4.1 新建工具类

在对应领域子目录下新增 `McpToolRenameFile.hpp` / `.cpp`。
文件头沿用仓库现有版权注释，类放在 `McpServer` 命名空间：

```cpp
#ifndef MCPTOOLRENAMEFILE_HPP
#define MCPTOOLRENAMEFILE_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief rename_file 工具：重命名项目内的文件。
//! @note Execute() 必须在 GUI 线程中调用。
class RenameFileTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;
};
} // namespace McpServer

#endif // MCPTOOLRENAMEFILE_HPP
```

实现中复用 `FileTools/McpFileToolSupport.hpp` 提供的路径规范化与项目归属校验：

```cpp
std::string RenameFileTool::Name() const { return "rename_file"; }

std::vector<ToolParameter> RenameFileTool::Parameters() const
{
   return {{"file", "原文件路径（须位于项目目录内）", true},
           {"newFile", "新文件路径（须位于项目目录内）", true},
           {"overwrite", "目标已存在时是否覆盖，默认 false", false}};
}

ToolTextResult RenameFileTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult result;
   std::string    error;

   if (!FileTools::RequireProject(error))
   {
      result.mMessage = error;
      return result;
   }

   const std::string source = FileTools::ResolveProjectPath(FileTools::Argument(aArguments, "file"), error);
   if (source.empty())
   {
      result.mMessage = error;
      return result;
   }
   // …校验目标路径、执行重命名，成功后设置 result.mSuccess = true
   return result;
}
```

### 4.2 注册到领域入口

在所在目录的注册函数里追加一行（文件类为 `FileTools/McpFileTools.cpp`，
平台类为 `source/McpScenarioTools.cpp`）：

```cpp
aRegistry.Register(std::make_unique<RenameFileTool>());
```

若属于全新的领域，可新建 `XxxTools.cpp/.hpp` 提供 `RegisterXxxTools(ToolRegistry&)`，
并在 `McpServerDock::StartServer()` 中调用一次。

### 4.3 重新构建

`source/CMakeLists.txt` 已递归收集子目录源码，新增 `.hpp/.cpp` 直接重新构建即可，
无需修改任何 CMake 文件。

### 4.4 验证

启动服务后调用 `tools/list` 应能看到新工具及其输入模式，再按参数调用 `tools/call`。
工具名必须全局唯一，否则会与已有工具冲突。

### 约定

- 一个工具类只做一件事，不要在一个类里合并多个操作。
- `Name()` 用小写下划线命名，`Title()` / `Description()` / 参数说明用面向用户的语言。
- `Parameters()` 中必填参数标 `true`；需要保留原始首尾空白的文本（如文件内容）
  把 `ToolParameter` 第四项设为 `true`。
- 失败时返回 `mSuccess = false` 并在 `mMessage` 中说明原因；成功时的 `mMessage`
  会被直接作为工具返回值，不要留空（留空会被判定为等待 GUI 超时）。
- 文件路径一律先经过 `FileTools::ResolveProjectPath()`，避免越出项目目录。
- 不要从 `Execute()` 里直接访问服务线程对象，也不要在工具中启动新的线程。
