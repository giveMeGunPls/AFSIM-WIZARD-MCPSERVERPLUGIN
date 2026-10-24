// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPFILETOOLSUPPORT_HPP
#define MCPFILETOOLSUPPORT_HPP

#include <string>

#include "McpToolRegistry.hpp"

namespace wizard
{
class TextSource;
}

namespace McpServer
{
namespace FileTools
{
//! 读取工具参数（不存在时返回空字符串）。
std::string Argument(const ToolArguments& aArguments, const std::string& aName);

//! 解析布尔型参数，缺省为 false。
bool ParseBoolean(const std::string& aValue, bool& aResult, std::string& aError);

//! 校验当前存在已打开的项目，不存在时写入 aError。
bool RequireProject(std::string& aError);

//! 当前项目是否存在未保存的文件修改。
bool HasUnsavedChanges();

//! 将绝对路径原样返回；相对路径按项目目录解析。
//! 解析结果必须位于项目目录内，否则写入 aError 并返回空字符串。
std::string ResolveProjectPath(const std::string& aPath, std::string& aError);

//! 判断路径是否被解析到项目目录内（用于诊断提示）。
bool IsInsideProject(const std::string& aAbsolutePath);

//! 判断两个路径是否指向同一文件（Windows 下忽略大小写）。
bool PathsEqual(const std::string& aLhsPath, const std::string& aRhsPath);

//! 原样按字节复制文件（不经过编辑器，保留行尾与首尾空白）。
bool CopyFileRaw(const std::string& aSourcePath, const std::string& aTargetPath, std::string& aError);

//! 判断路径是否存在（文件或目录）。
bool PathExists(const std::string& aAbsolutePath);

//! 校验目标路径的父目录存在，失败时写入 aError。
bool EnsureParentDirectory(const std::string& aAbsolutePath, std::string& aError);

//! 查找已经载入项目缓存中的文本文件，不存在时返回 nullptr。
wizard::TextSource* FindExistingSource(const std::string& aAbsolutePath);

//! 返回当前活动编辑器对应的文本文件，没有时返回 nullptr。
wizard::TextSource* CurrentSource();

//! 确保文本文件已载入内容，失败时写入 aError。
bool EnsureLoaded(wizard::TextSource* aSourcePtr, std::string& aError);
} // namespace FileTools
} // namespace McpServer

#endif
