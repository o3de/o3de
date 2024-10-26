// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

namespace ${SanitizedCppName}
{
    // System Component TypeIds
    inline constexpr const char* ${SanitizedCppName}SystemComponentTypeId = "{${Random_Uuid}}";
    inline constexpr const char* ${SanitizedCppName}EditorSystemComponentTypeId = "{${Random_Uuid}}";

    // Module derived classes TypeIds
    inline constexpr const char* ${SanitizedCppName}ModuleInterfaceTypeId = "{${Random_Uuid}}";
    inline constexpr const char* ${SanitizedCppName}ModuleTypeId = "{${Random_Uuid}}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* ${SanitizedCppName}EditorModuleTypeId = ${SanitizedCppName}ModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* ${SanitizedCppName}RequestsTypeId = "{${Random_Uuid}}";
} // namespace ${SanitizedCppName}
