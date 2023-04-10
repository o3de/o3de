/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace ${Name}
{
    // System Component TypeIds
    inline constexpr const char* ${Name}SystemComponentTypeId = "${SysCompClassId}";
    inline constexpr const char* ${Name}EditorSystemComponentTypeId = "${EditorSysCompClassId}";

    // Module derived classes TypeIds
    inline constexpr const char* ${Name}ModuleInterfaceTypeId = "{${Random_Uuid}}";
    inline constexpr const char* ${Name}ModuleTypeId = "${ModuleClassId}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* ${Name}EditorModuleTypeId = ${SanitizedCppName}ModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* ${Name}RequestsTypeId = "{${Random_Uuid}}";
} // namespace ${Name}
