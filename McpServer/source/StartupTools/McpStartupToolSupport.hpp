// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPSTARTUPTOOLSUPPORT_HPP
#define MCPSTARTUPTOOLSUPPORT_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"
#include "UtPath.hpp"

namespace McpServer
{
namespace StartupTools
{
//! 校验参数中的启动文件路径：必须存在、为文本文件，且位于项目目录内。
//! @param aPathArgument 用户传入的路径（绝对路径或相对项目目录）
//! @param aAbsolutePath 成功时写入规范化后的绝对路径
//! @param aError        失败时写入错误说明
bool PrepareStartupFile(const std::string& aPathArgument, std::string& aAbsolutePath, std::string& aError);

//! 判断两个启动文件是否指向同一路径（Windows 下忽略大小写）。
bool StartupPathsEqual(const UtPath& aLhs, const UtPath& aRhs);

//! 判断两个启动文件列表是否完全一致（含顺序）。
bool SameStartupFiles(const std::vector<UtPath>& aLhs, const std::vector<UtPath>& aRhs);

//! 生成启动文件列表的可读文本：按执行顺序，第 1 项标注为主启动文件。
std::string DescribeStartupFiles(const std::vector<UtPath>& aFiles);

//! 解析可选的 save 参数；未提供时默认为 true。
bool ParseSaveArgument(const ToolArguments& aArguments, bool& aSave, std::string& aError);

//! 按 save 决定是否把启动文件列表写入项目文件，附加说明写入 aNote。
//! @note 项目尚未绑定项目文件时不会调用 SaveProject（该路径会弹出模态另存为对话框）。
bool SaveProjectIfRequested(bool aSave, std::string& aNote, std::string& aError);
} // namespace StartupTools
} // namespace McpServer

#endif
