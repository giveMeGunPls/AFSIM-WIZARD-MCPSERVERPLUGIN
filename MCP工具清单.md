# McpServer 工具清单

> 统计范围：`wizard/plugins/McpServer/source` 下所有通过 `aRegistry.Register(...)` 注册的 `McpTool` 子类。
> 当前共 **5 个领域、41 个工具**。

## 1. 总览

| 领域 | 目录 | 注册函数 | 工具数 |
| --- | --- | --- | --- |
| 文件与项目 | `source/FileTools` | `RegisterFileTools` | 10 |
| 场景与平台 | `source/PlatformTools` | `RegisterScenarioTools` | 20 |
| 解析校验 | `source/ParseTools` | `RegisterParseTools` | 1 |
| 启动文件 | `source/StartupTools` | `RegisterStartupTools` | 6 |
| 全局仿真配置 | `source/ConfigTools` | `RegisterConfigTools` | 4 |
| **合计** | | | **41** |

注册入口在 [McpServerDock.cpp](file:///e:/afsim/swdev_XY/swdev/src/wizard/plugins/McpServer/source/McpServerDock.cpp#L204-L209)，五个领域注册函数按顺序调用；工具元数据由 [McpToolRegistry.cpp](file:///e:/afsim/swdev_XY/swdev/src/wizard/plugins/McpServer/source/McpToolRegistry.cpp#L182-L200) 统一铺进 MCP `ServerBuilder`。

## 2. 文件与项目（10 个）

| # | 工具名 | 作用 | 参数（必填加粗） |
| --- | --- | --- | --- |
| 1 | `new_file` | 新建文本文件并打开到编辑器 | **`file`**、`content`（保留首尾空白） |
| 2 | `open_file` | 打开项目中已存在的文件 | **`file`** |
| 3 | `save_file` | 保存文件；未修改时不重写磁盘 | `file`（缺省当前活动文件） |
| 4 | `save_file_as` | 另存为；未修改时按原始字节复制 | `file`、**`newFile`**、`overwrite` |
| 5 | `save_all` | 保存项目中全部已修改文件 | 无 |
| 6 | `print_file` | 打印文件；指定 `output` 时输出 PDF | `file`、`output`（缺省系统打印机） |
| 7 | `new_project` | 新建项目，须为绝对路径且以 `.afproj` 结尾 | **`file`** |
| 8 | `close_project` | 关闭当前项目 | 无 |
| 9 | `save_project` | 保存项目文件 | `file`（项目未绑定时必填） |
| 10 | `export_project` | 导出项目包含的文件 | **`directory`**（须已存在）、`overwrite` |

实现文件：`FileTools/McpTool*.cpp`，公共路径校验在 `FileTools/McpFileToolSupport.*`。

## 3. 场景与平台（20 个）

### 3.1 平台（6 个）

| # | 工具名 | 作用 | 参数（必填加粗） |
| --- | --- | --- | --- |
| 11 | `list_platform_types` | 列出可用平台类型、已有平台名与默认目标脚本 | 无 |
| 12 | `add_platform` | 向目标脚本追加平台定义 | **`name`**、**`type`**、`file`、`latitude`、`longitude`、`side`、`icon`、`altitude`、`heading`、`pitch`、`roll` |
| 13 | `delete_platform` | 删除平台的全部脚本定义块 | **`name`** |
| 14 | `clone_platform` | 复制平台定义并改名后追加到同一脚本 | **`name`**、`new_name`（缺省 `<name>_1`） |
| 15 | `find_platform` | 返回平台所在文件、行号、类型与定义原文 | **`name`** |
| 16 | `set_platform_attributes` | 就地修改平台顶层属性行 | **`name`**、`side`、`icon`、`latitude`、`longitude`、`altitude`、`heading`、`pitch`、`roll` |

### 3.2 平台部件（4 个）

| # | 工具名 | 作用 | 参数（必填加粗） |
| --- | --- | --- | --- |
| 17 | `list_platform_part_types` | 按类别列出部件可用类型（脚本定义 + 内置） | `kind`（sensor/weapon/comm/processor/mover/fuel） |
| 18 | `list_platform_parts` | 列出平台部件（类别、名称、类型、行号） | **`platform`** |
| 19 | `add_platform_part` | 向平台插入部件块 | **`platform`**、**`kind`**、**`type`**、`name` |
| 20 | `remove_platform_part` | 删除平台部件块 | **`platform`**、`name`、`kind` |

### 3.3 航线与航路点（5 个）

| # | 工具名 | 作用 | 参数（必填加粗） |
| --- | --- | --- | --- |
| 21 | `add_route` | 为平台添加航线块，或追加全局航线 | `platform`、`scope`、`route`、`file`、`latitude`、`longitude`、`altitude`、`speed` |
| 22 | `add_waypoint` | 在航线末尾追加航路点 | **`latitude`**、**`longitude`**、`platform`、`scope`、`route`、`altitude`、`speed`、`label` |
| 23 | `update_waypoint` | 按从 0 开始的索引修改航路点 | **`index`**、`platform`、`scope`、`route`、`latitude`、`longitude`、`altitude`、`speed`、`label` |
| 24 | `remove_waypoint` | 按索引删除航路点 | **`index`**、`platform`、`scope`、`route` |
| 25 | `remove_route` | 删除平台航线块或全局航线 | `platform`、`scope`、`route` |

### 3.4 区域（3 个）

| # | 工具名 | 作用 | 参数（必填加粗） |
| --- | --- | --- | --- |
| 26 | `add_zone` | 向目标脚本追加区域定义 | **`name`**、`latitude`、`longitude`、`type`、`file`、`heading`、`minimum_altitude`、`maximum_altitude`、`minimum_radius`、`maximum_radius`、`lateral_axis`、`longitudinal_axis`、`start_angle`、`stop_angle`、`fill_color`、`line_color`、`points` |
| 27 | `find_zone` | 返回区域所在文件、行号、形状与定义原文 | **`name`** |
| 28 | `remove_zone` | 删除区域定义 | **`name`** |

补充约束：

- `add_zone` 的 `type` 取 `circular`（默认）、`spherical`、`elliptical`、`polygonal`；非多边形需 `latitude` + `longitude`，多边形需 `points`（`lat lon; lat lon; ...`）。
- 航线类工具的 `scope` 缺省为 `platform`，全局航线需 `scope: "global"` 并由 `route` 指定名称。
- 这些工具只改脚本原文（追加或区间替换），由 Wizard 重新解析生成场景对象，不操作 GUI 对象。

### 3.5 平台资源（2 个）

| # | 工具名 | 作用 | 参数（必填加粗） |
| --- | --- | --- | --- |
| 29 | `list_sides` | 列出平台数据面板中可选的阵营（side）名称 | `side`（可选，只查询单个阵营） |
| 30 | `list_icons` | 按分类列出模型数据库中全部图标（icon）名称 | `category`（可选，只查询单个分类） |

说明：`list_sides` 的阵营名直接取自 `UtColor::GetColorMap()`；`list_icons` 取自 `VaModelDatabase` 的模型定义并按 `VaModelDefinition::GetCategories()` 分组，未设置分类的图标归入 `uncategorized`。两者与 PlatformData 插件中 Side / Icon 下拉框的数据源一致，但均不依赖已打开的项目。

## 4. 解析校验（1 个）

| # | 工具名 | 作用 | 参数（必填加粗） |
| --- | --- | --- | --- |
| 31 | `validate_input` | 用当前项目语法定义独立解析一段 AFSIM 输入块，返回无法匹配的命令 | **`input`**（保留首尾空白） |

说明：使用独立 `WsfParser` 在内存文档上解析，不读写文件、不改场景。仅校验输入块语法，脚本函数体（`script ... end_script`）不在校验范围内。

## 5. 启动文件（6 个）

| # | 工具名 | 作用 | 参数（必填加粗） |
| --- | --- | --- | --- |
| 32 | `get_startup_files` | 列出启动文件（顺序、主文件标记、路径、是否存在） | 无 |
| 33 | `get_current_startup_file` | 返回主启动文件（列表第 1 项） | 无 |
| 34 | `set_current_startup_file` | 用单个文件替换启动文件列表 | **`file`**、`save`（缺省 true） |
| 35 | `add_startup_file` | 追加到启动文件列表末尾 | **`file`**、`save`（缺省 true） |
| 36 | `remove_startup_file` | 从列表移除一项；不允许移除唯一项 | **`file`**、`save`（缺省 true） |
| 37 | `set_startup_files` | 按顺序整体替换列表 | **`files`**（每行一个，不允许重复）、`save`（缺省 true） |

说明：直接读写 `wizard::Project` 的启动文件列表，不接触 UI；`save=false` 只改内存。

## 6. 全局仿真配置（4 个）

| # | 工具名 | 作用 | 参数（必填加粗） |
| --- | --- | --- | --- |
| 38 | `set_log_file` | 生成 `log_file <path>` | **`path`**、`file`（缺省主启动文件） |
| 39 | `set_event_output` | 生成 `event_output ... end_event_output` 块 | **`path`**、`file` |
| 40 | `set_event_pipe` | 生成 `event_pipe ... end_event_pipe` 块 | **`path`**、`preset`（缺省 `full`）、`file` |
| 41 | `set_end_time` | 生成 `end_time <time>` | **`time`**（如 `1 hour`）、`file` |

说明：单行命令已有同名顶层命令时就地替换；`event_output` / `event_pipe` 已有顶层块时只更新 `file`（及 `use_preset`）参数行，保留块内其他设置。

## 7. 通用约定

- 所有参数在注册表层统一按字符串解析，未声明的参数会被直接拒绝（见 [McpToolRegistry.cpp](file:///e:/afsim/swdev_XY/swdev/src/wizard/plugins/McpServer/source/McpToolRegistry.cpp#L76-L125)）。
- 文件路径一律限制在当前项目目录内，绝对路径或 `..` 越界会被拒绝。
- 工具 `Execute()` 由服务线程投递到 GUI 线程执行，最多等待 5 秒，因此内部不能弹模态框或长时间阻塞。
- 失败返回 `mSuccess = false` 并在 `mMessage` 中说明原因；成功时 `mMessage` 直接作为工具返回值。

## 8. 相关文件

| 内容 | 路径 |
| --- | --- |
| 工具基类与注册表 | `source/McpToolRegistry.hpp/.cpp` |
| 文件类注册 | `source/FileTools/McpFileTools.cpp` |
| 场景类注册 | `source/McpScenarioTools.cpp` |
| 解析类注册 | `source/McpParseTools.cpp` |
| 启动文件类注册 | `source/StartupTools/McpStartupTools.cpp` |
| 配置类注册 | `source/ConfigTools/McpConfigTools.cpp` |
| 服务面板与 GUI 执行器 | `source/McpServerDock.cpp` |
| 场景编辑公共支持 | `source/McpScenarioEditSupport.hpp/.cpp` |
