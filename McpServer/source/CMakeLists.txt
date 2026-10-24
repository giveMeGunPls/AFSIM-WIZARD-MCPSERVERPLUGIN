# ****************************************************************************
# CUI
#
# The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
#
# The use, dissemination or disclosure of data in this file is subject to
# limitation or restriction. See accompanying README and LICENSE for details.
# ****************************************************************************
# 工具已按文件拆分（McpTool*.cpp / McpScenario*.cpp），由模板中的 file(GLOB) 自动纳入构建
plugin_cmakelists_template(WizMcpServer wizard
                           LIBRARIES wizard_core wkf_common)

# 模板中的 file(GLOB) 不递归子目录，这里补充各工具子目录（FileTools、PlatformTools 等）中的源码
# CONFIGURE_DEPENDS 让新增子目录或源文件在下次构建时自动重新配置，无需手工重跑 CMake
file(GLOB MCP_TOOL_SUBDIRS LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*")
set(MCP_TOOL_SUBDIR_SOURCES "")
set(MCP_TOOL_SUBDIR_INCLUDES "")
foreach(MCP_TOOL_SUBDIR ${MCP_TOOL_SUBDIRS})
   if(IS_DIRECTORY "${MCP_TOOL_SUBDIR}")
      file(GLOB_RECURSE MCP_TOOL_SOURCES CONFIGURE_DEPENDS
           "${MCP_TOOL_SUBDIR}/*.cpp"
           "${MCP_TOOL_SUBDIR}/*.hpp")
      list(APPEND MCP_TOOL_SUBDIR_SOURCES ${MCP_TOOL_SOURCES})
      list(APPEND MCP_TOOL_SUBDIR_INCLUDES "${MCP_TOOL_SUBDIR}")
   endif()
endforeach()
if(MCP_TOOL_SUBDIR_SOURCES)
   target_sources(WizMcpServer PRIVATE ${MCP_TOOL_SUBDIR_SOURCES})
   source_group("ToolSources" FILES ${MCP_TOOL_SUBDIR_SOURCES})
endif()

# 子目录中的源码需要能按文件名引用上级目录的头文件（如 McpToolRegistry.hpp）
target_include_directories(WizMcpServer PRIVATE
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           ${MCP_TOOL_SUBDIR_INCLUDES})

# cxxmcp 头文件需要 C++17；PRIVATE 只作用于本插件自身，不影响其他目标
target_compile_features(WizMcpServer PRIVATE cxx_std_17)

# MSVC 下源码与执行字符集都按 UTF-8 处理，避免界面中的中文乱码
if(MSVC)
   target_compile_options(WizMcpServer PRIVATE /utf-8)
endif()

# cxxmcp SDK 提供预编译静态库，头文件只有 Release 版，库区分 Debug / Release
set(CXXMCP_SDK_ROOT "E:/cxxmcp_sdk" CACHE PATH "cxxmcp SDK root directory")

target_include_directories(WizMcpServer PRIVATE "${CXXMCP_SDK_ROOT}/Release/include")

# mcp_server.lib 内置的 httplib 带 SSL 支持，链接时需要用 OpenSSL 补齐符号
# 这里用静态库（/MD 版），避免运行时再部署 libcrypto-3-x64.dll / libssl-3-x64.dll
set(OPENSSL_SDK_ROOT "D:/OpenSSL-Win64" CACHE PATH "OpenSSL root directory")

target_include_directories(WizMcpServer PRIVATE "${OPENSSL_SDK_ROOT}/include")

# plugin_cmakelists_template 已用 plain 签名链接过该目标，这里必须保持一致
target_link_libraries(WizMcpServer
                      "${CXXMCP_SDK_ROOT}/Release/lib/mcp_server.lib"
                      "${CXXMCP_SDK_ROOT}/Release/lib/mcp_protocol.lib"
                      "${OPENSSL_SDK_ROOT}/lib/VC/static/libssl64MD.lib"
                      "${OPENSSL_SDK_ROOT}/lib/VC/static/libcrypto64MD.lib"
                      ws2_32.lib
                      crypt32.lib
                      advapi32.lib
                      user32.lib)
