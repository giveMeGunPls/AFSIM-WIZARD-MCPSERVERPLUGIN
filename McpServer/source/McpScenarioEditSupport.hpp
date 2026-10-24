// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPSCENARIOEDITSUPPORT_HPP
#define MCPSCENARIOEDITSUPPORT_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "McpToolRegistry.hpp"
#include "WsfPProxyNode.hpp"

namespace wizard
{
class TextSource;
}

namespace McpServer
{
namespace ScenarioEdit
{
//! 脚本中的一行文本；所有偏移均为文件内的绝对偏移
struct TextLine
{
   size_t      mBegin             = 0;     //!< 行首字符偏移
   size_t      mEnd               = 0;     //!< 行内容最后一个字符；空行为 mBegin - 1
   size_t      mContentEnd        = 0;     //!< 行内容最后一个字符（与 mEnd 相同，保留语义）
   size_t      mIndent            = 0;     //!< 前导空格数
   bool        mHasNewline        = false; //!< 是否以 '\n' 结束
   std::string mText;                      //!< 不含换行符的行内容
};

//! 按行拆分文本，aBaseOffset 为文本首字符在文件中的绝对偏移
//! @note 文档在内存中已把 CRLF 归一化为 '\n'，因此这里只处理 '\n'
std::vector<TextLine> SplitLines(const std::string& aText, size_t aBaseOffset);

//! 读取工具参数；未提供时返回空串
std::string Argument(const ToolArguments& aArguments, const std::string& aName);

//! 编辑器偏好中的缩进单位（TabSpace 个空格）
std::string IndentUnit();

//! 拆分一行中的空白分隔词
std::vector<std::string> SplitTokens(const std::string& aText);

//! 解析非负十进制序号；空串或含非数字字符时返回 false
bool ParseIndex(const std::string& aValue, size_t& aIndex);

//! 当前项目已打开且场景已解析，proxy 可用
bool RequireParsedProject(std::string& aError);

//! 按名称查找平台代理节点
bool FindPlatform(const std::string& aName, WsfPProxyNode& aNode, std::string& aError);

//! 取得代理节点对应的定义文本及其在文件中的区间（区间两端都含）
bool GetDefinition(const WsfPProxyNode& aNode,
                   wizard::TextSource*& aSource,
                   std::string&         aText,
                   size_t&              aBegin,
                   size_t&              aEnd,
                   std::string&         aError);

//! 文件正文最后一个字符的偏移（文档末尾带 '\0' 结束符）
size_t TextEndIndex(wizard::TextSource* aSource);

//! 相对项目目录的显示用路径
std::string RelativePath(const std::string& aAbsolutePath);

//! 一次编辑；mEnd == mBegin - 1 表示在该位置纯插入（区间两端都含）
struct Edit
{
   size_t      mBegin = 0;
   size_t      mEnd   = 0;
   std::string mText;
};

//! 按起点从后向前应用编辑，避免偏移失效
bool ApplyEdits(wizard::TextSource* aSource, std::vector<Edit> aEdits, std::string& aError);

//! 在文件末尾追加文本（调用方负责提供前导换行）
bool AppendText(wizard::TextSource* aSource, const std::string& aText, std::string& aError);

//! 判断行（忽略缩进）是否以指定关键字开头且关键字后为空白或行尾
bool LineHasKeyword(const TextLine& aLine, const std::string& aKeyword);

//! 行尾字符偏移：含换行符时为 '\n' 所在位置，否则为行内最后一个字符
size_t LineEndIncludingNewline(const TextLine& aLine);

//! 生成一条航路点的脚本文本（label/position 同级，speed 再缩进一级）
std::string BuildWaypointText(const std::string& aWaypointIndent,
                              const std::string& aIndent,
                              const std::string& aLabel,
                              const std::string& aLatitude,
                              const std::string& aLongitude,
                              const std::string& aAltitude,
                              const std::string& aSpeed);

//! 航线块位置信息
struct RouteLocation
{
   wizard::TextSource* mSource      = nullptr;
   std::string         mText;                  //!< 航线块文本（含 route...end_route）
   size_t              mBegin       = 0;       //!< 块首字符绝对偏移
   size_t              mEnd         = 0;       //!< 块末字符绝对偏移（含换行）
   size_t              mStartIndent = 0;       //!< route 行的缩进
};

//! 定位航线块：scope 为 "platform" 时从平台定义内查找，为 "global" 时按全局航线名查找
bool LocateRouteBlock(const std::string& aPlatform,
                      const std::string& aScope,
                      const std::string& aRouteName,
                      RouteLocation&     aLocation,
                      std::string&       aError);

//! 在航线块内查找 end_route 行下标；找不到返回 false
bool FindEndRouteLine(const std::vector<TextLine>& aLines, size_t aStartIndent, size_t& aIndex);

//! 收集航线块内的航路点（position 行下标），按出现顺序排列
std::vector<size_t> FindWaypointPositions(const std::vector<TextLine>& aLines, size_t aStartIndent);
} // namespace ScenarioEdit
} // namespace McpServer

#endif
