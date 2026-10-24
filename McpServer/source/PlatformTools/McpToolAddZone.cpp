// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "McpToolAddZone.hpp"

#include <set>
#include <string>
#include <vector>

#include "FileTools/McpFileToolSupport.hpp"
#include "McpScenarioEditSupport.hpp"
#include "McpScenarioQuery.hpp"
#include "Project.hpp"
#include "ProjectWorkspace.hpp"
#include "TextSource.hpp"
#include "WsfPM_Root.hpp"
#include "WsfPProxy.hpp"
#include "WsfPProxyObjectMap.hpp"

namespace
{
const std::set<std::string>& ZoneTypes()
{
   static const std::set<std::string> types = {"circular", "spherical", "elliptical", "polygonal"};
   return types;
}
} // namespace

namespace McpServer
{
std::string AddZoneTool::Name() const
{
   return "add_zone";
}

std::string AddZoneTool::Title() const
{
   return "添加区域";
}

std::string AddZoneTool::Description() const
{
   return "向想定的脚本文件追加区域定义（默认为圆形，也可为球形、椭圆形或多边形）。"
          "Wizard 重新解析脚本后，该区域即成为想定的一部分。";
}

std::vector<ToolParameter> AddZoneTool::Parameters() const
{
   return {
      {"name", "唯一的区域名称。", true},
      {"latitude", "区域中心纬度，如 '27:52:46.72'。多边形区域不使用此项。", false},
      {"longitude", "区域中心经度，如 '118:01:01.03'。多边形区域不使用此项。", false},
      {"type", "区域形状：circular（默认）、spherical、elliptical 或 polygonal。", false},
      {"file",
       "目标脚本文件，相对于项目目录。缺省使用包含平台定义最多的文件。",
       false},
      {"heading", "带单位的区域朝向，如 '0 deg'。", false},
      {"minimum_altitude", "最低高度，如 '0 m'。默认 '0 m'。", false},
      {"maximum_altitude", "最高高度，如 '100 km'。默认 '100 km'。", false},
      {"minimum_radius", "圆形区域的最小半径。默认 '0 m'。", false},
      {"maximum_radius", "圆形/球形区域的最大半径。默认 '10 km'。", false},
      {"lateral_axis", "椭圆形区域的横向轴长，如 '10 km'。默认 '10 km'。", false},
      {"longitudinal_axis", "椭圆形区域的纵向轴长，如 '20 km'。默认 '20 km'。", false},
      {"start_angle", "起始角，如 '0 deg'。默认 '0 deg'。", false},
      {"stop_angle", "终止角，如 '360 deg'。默认 '360 deg'。", false},
      {"fill_color", "填充颜色，格式为 'R G B A'。默认 '0 0 0 64'。", false},
      {"line_color", "线条颜色，格式为 'R G B'。默认 '0 0 0'。", false},
      {"points",
       "多边形顶点，格式为 'lat lon; lat lon; ...'。type 为 'polygonal' 时必填。", false},
   };
}

bool AddZoneTool::AppendPoints(std::string&       aZoneDef,
                               const std::string& aIndent,
                               const std::string& aPoints,
                               std::string&       aError)
{
   size_t start = 0;
   while (start <= aPoints.size())
   {
      const size_t separator = aPoints.find(';', start);
      const std::string pair = aPoints.substr(start, (separator == std::string::npos ? aPoints.size() : separator) - start);

      const std::vector<std::string> tokens = ScenarioEdit::SplitTokens(pair);
      if (tokens.size() != 2)
      {
         aError = "Each polygon vertex must be 'latitude longitude', separated by ';'.";
         return false;
      }
      aZoneDef += aIndent + "point " + tokens[0] + " " + tokens[1] + "\n";

      if (separator == std::string::npos)
      {
         break;
      }
      start = separator + 1;
   }
   return true;
}

ToolTextResult AddZoneTool::Execute(const ToolArguments& aArguments) const
{
   ToolTextResult    result;
   const std::string name         = ScenarioEdit::Argument(aArguments, "name");
   const std::string latitude     = ScenarioEdit::Argument(aArguments, "latitude");
   const std::string longitude    = ScenarioEdit::Argument(aArguments, "longitude");
   const std::string type         = ScenarioEdit::Argument(aArguments, "type");
   const std::string file         = ScenarioEdit::Argument(aArguments, "file");
   const std::string heading      = ScenarioEdit::Argument(aArguments, "heading");
   const std::string minAltitude  = ScenarioEdit::Argument(aArguments, "minimum_altitude");
   const std::string maxAltitude  = ScenarioEdit::Argument(aArguments, "maximum_altitude");
   const std::string minRadius    = ScenarioEdit::Argument(aArguments, "minimum_radius");
   const std::string maxRadius    = ScenarioEdit::Argument(aArguments, "maximum_radius");
   const std::string lateralAxis  = ScenarioEdit::Argument(aArguments, "lateral_axis");
   const std::string longitudinal = ScenarioEdit::Argument(aArguments, "longitudinal_axis");
   const std::string startAngle   = ScenarioEdit::Argument(aArguments, "start_angle");
   const std::string stopAngle    = ScenarioEdit::Argument(aArguments, "stop_angle");
   const std::string fillColor    = ScenarioEdit::Argument(aArguments, "fill_color");
   const std::string lineColor    = ScenarioEdit::Argument(aArguments, "line_color");
   const std::string points       = ScenarioEdit::Argument(aArguments, "points");

   std::string error;
   if (!ScenarioEdit::RequireParsedProject(error))
   {
      result.mMessage = error;
      return result;
   }

   if (name.find_first_of(" \t\r\n/#") != std::string::npos)
   {
      result.mMessage = "Zone name must not contain spaces, '/', or '#'.";
      return result;
   }

   const std::string shape = type.empty() ? "circular" : type;
   if (ZoneTypes().find(shape) == ZoneTypes().end())
   {
      result.mMessage = "Unknown zone type '" + shape + "'. Use circular, spherical, elliptical, or polygonal.";
      return result;
   }
   if ((shape == "polygonal") && points.empty())
   {
      result.mMessage = "A polygonal zone requires 'points'.";
      return result;
   }
   if ((shape != "polygonal") && (latitude.empty() || longitude.empty()))
   {
      result.mMessage = "Provide 'latitude' and 'longitude' for the zone center.";
      return result;
   }
   if (WsfPM_Root(wizard::Project::Instance()->Proxy()).zones().Find(name).IsValid())
   {
      result.mMessage = "A zone named '" + name + "' already exists.";
      return result;
   }

   const std::string relativeFile = file.empty() ? ScenarioQuery::DefaultTargetFile() : file;
   if (relativeFile.empty())
   {
      result.mMessage = "No target script file is available in this project; pass 'file' explicitly.";
      return result;
   }

   std::string pathError;
   const std::string absoluteFile = FileTools::ResolveProjectPath(relativeFile, pathError);
   if (absoluteFile.empty())
   {
      result.mMessage = pathError;
      return result;
   }
   wizard::TextSource* source = wizard::ProjectWorkspace::Instance()->FindSource(absoluteFile, false, false);
   if (!source)
   {
      result.mMessage = "Unable to find the target script file: " + relativeFile;
      return result;
   }

   const std::string indent       = ScenarioEdit::IndentUnit();
   const std::string nestedIndent = indent + indent;

   std::string zoneDef = "zone " + name + "\n";
   zoneDef += indent + shape + "\n";
   zoneDef += indent + "fill_color " + (fillColor.empty() ? "0 0 0 64" : fillColor) + "\n";
   zoneDef += indent + "line_color " + (lineColor.empty() ? "0 0 0" : lineColor) + "\n";
   zoneDef += nestedIndent + "minimum_altitude " + (minAltitude.empty() ? "0 m" : minAltitude) + "\n";
   zoneDef += nestedIndent + "maximum_altitude " + (maxAltitude.empty() ? "100 km" : maxAltitude) + "\n";

   if (shape == "polygonal")
   {
      // 绝对坐标多边形：先声明 lat_lon，再逐点给出经纬度
      zoneDef += nestedIndent + "lat_lon\n";
      if (!AppendPoints(zoneDef, nestedIndent, points, error))
      {
         result.mMessage = error;
         return result;
      }
   }
   else
   {
      zoneDef += nestedIndent + "position " + latitude + " " + longitude + "\n";
      if (!heading.empty())
      {
         zoneDef += nestedIndent + "heading " + heading + "\n";
      }

      if (shape == "circular")
      {
         zoneDef += nestedIndent + "minimum_radius " + (minRadius.empty() ? "0 m" : minRadius) + "\n";
         zoneDef += nestedIndent + "maximum_radius " + (maxRadius.empty() ? "10 km" : maxRadius) + "\n";
      }
      else if (shape == "spherical")
      {
         zoneDef += nestedIndent + "maximum_radius " + (maxRadius.empty() ? "10 km" : maxRadius) + "\n";
      }
      else
      {
         zoneDef += nestedIndent + "lateral_axis " + (lateralAxis.empty() ? "10 km" : lateralAxis) + "\n";
         zoneDef += nestedIndent + "longitudinal_axis " + (longitudinal.empty() ? "20 km" : longitudinal) + "\n";
      }

      zoneDef += nestedIndent + "start_angle " + (startAngle.empty() ? "0 deg" : startAngle) + "\n";
      zoneDef += nestedIndent + "stop_angle " + (stopAngle.empty() ? "360 deg" : stopAngle) + "\n";
   }
   zoneDef += "end_zone\n";

   if (!ScenarioEdit::AppendText(source, "\n" + zoneDef, error))
   {
      result.mMessage = error;
      return result;
   }

   result.mSuccess = true;
   result.mMessage = "Created zone '" + name + "' (" + shape + ") in '" + relativeFile + "'.\n\n" + zoneDef;
   return result;
}
} // namespace McpServer
