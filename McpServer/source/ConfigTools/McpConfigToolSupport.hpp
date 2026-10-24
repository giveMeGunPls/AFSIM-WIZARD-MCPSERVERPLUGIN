// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPCONFIGTOOLSUPPORT_HPP
#define MCPCONFIGTOOLSUPPORT_HPP

#include <string>
#include <vector>

namespace wizard
{
class TextSource;
}

namespace McpServer
{
namespace ConfigTools
{
//! 块内需要设置的参数行；mKeyword 为行首关键字，mText 为不含缩进与换行的完整行内容
struct BlockOption
{
   std::string mKeyword;
   std::string mText;
};

//! 校验单值参数：不能为空，且必须是单行文本
bool ValidateValue(const std::string& aName, const std::string& aValue, std::string& aError);

//! 确定目标脚本并取得其 TextSource。
//! @param aFileArgument 可选的目标脚本（绝对路径或相对项目目录）；为空时优先取主启动文件，
//!                      没有启动文件时回退到平台定义最多的脚本
//! @param aSource       成功时写入目标脚本的 TextSource
//! @param aTargetRelative 成功时写入目标脚本相对项目目录的路径，用于结果展示
bool ResolveTargetSource(const std::string& aFileArgument,
                         wizard::TextSource*& aSource,
                         std::string&         aTargetRelative,
                         std::string&         aError);

//! 写入单行命令：已存在同名的顶层命令行时就地替换，否则追加到文件末尾
//! @param aKeyword     顶层命令关键字，如 "log_file"
//! @param aCommandText 完整命令行（含结尾换行）
bool UpsertCommand(wizard::TextSource* aSource,
                   const std::string& aKeyword,
                   const std::string& aCommandText,
                   std::string&       aError);

//! 写入带结束标签的命令块：已存在同名块时只更新块内参数行并保留其他内容，否则追加整块
//! @param aBlockText 块不存在时追加的完整块文本（含结尾换行）
bool UpsertBlock(wizard::TextSource*             aSource,
                 const std::string&               aStartKeyword,
                 const std::string&               aEndKeyword,
                 const std::string&               aBlockText,
                 const std::vector<BlockOption>&  aOptions,
                 std::string&                     aError);
} // namespace ConfigTools
} // namespace McpServer

#endif // MCPCONFIGTOOLSUPPORT_HPP
