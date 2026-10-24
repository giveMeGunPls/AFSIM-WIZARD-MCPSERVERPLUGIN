// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef MCPTOOLADDPLATFORM_HPP
#define MCPTOOLADDPLATFORM_HPP

#include <string>
#include <vector>

#include "McpToolRegistry.hpp"

namespace McpServer
{
//! @brief add_platform 工具：向当前场景的脚本文件追加平台定义。
//! @note Execute() 必须在 GUI 线程中调用。
class AddPlatformTool : public McpTool
{
public:
   std::string                Name() const override;
   std::string                Title() const override;
   std::string                Description() const override;
   std::vector<ToolParameter> Parameters() const override;
   ToolTextResult             Execute(const ToolArguments& aArguments) const override;

private:
   //! 工具参数对应的请求模型
   struct Request
   {
      std::string mName;
      std::string mType;
      std::string mFile;
      std::string mLatitude;
      std::string mLongitude;
      std::string mSide;
      std::string mIcon;
      std::string mAltitude;
      std::string mHeading;
      std::string mPitch;
      std::string mRoll;
   };

   //! 把工具参数转成请求并做语义校验；必填项由注册表统一校验
   static bool ParseRequest(const ToolArguments& aArguments, Request& aRequest, std::string& aError);

   //! 使用编辑器偏好中的 TabSpace 作为属性行缩进
   static std::string AttributeIndent();

   //! 属性值留空时不生成对应行
   static void AppendAttribute(std::string&       aPlatformDef,
                               const std::string& aIndent,
                               const char*        aKeyword,
                               const std::string& aValue);
};
} // namespace McpServer

#endif // MCPTOOLADDPLATFORM_HPP
