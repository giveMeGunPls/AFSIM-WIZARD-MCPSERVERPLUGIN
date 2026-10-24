// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpParseTools.hpp"

#include <memory>

#include "ParseTools/McpToolValidateInput.hpp"

namespace McpServer
{
void RegisterParseTools(ToolRegistry& aRegistry)
{
   // 输入块的语法校验
   aRegistry.Register(std::make_unique<ValidateInputTool>());
}
} // namespace McpServer
