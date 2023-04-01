
#pragma once

namespace ${Name}
{
    // System Component TypeIds
    inline constexpr const char* ${Name}SystemComponentTypeId = "${SysCompClassId}";
    inline constexpr const char* ${Name}EditorSystemComponentTypeId = "${EditorSysCompClassId}";

    // Module derived classes TypeIds
    inline constexpr const char* ${Name}ModuleInterfaceTypeId = "{E22C9D32-526E-481F-8440-29E489EEB349}";
    inline constexpr const char* ${Name}ModuleTypeId = "${ModuleClassId}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* ${Name}EditorModuleTypeId = ${Name}ModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* ${Name}RequestsTypeId = "{D6D4B90B-2392-40D1-8177-9C71383B57AD}";
} // namespace ${Name}
