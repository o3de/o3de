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
    inline constexpr const char* ${Name}SystemComponentTypeId = "{${Random_Uuid}}";
    inline constexpr const char* ${Name}EditorSystemComponentTypeId = "{${Random_Uuid}}";

    // Module derived classes TypeIds
    inline constexpr const char* ${Name}ModuleInterfaceTypeId = "{${Random_Uuid}}";
    inline constexpr const char* ${Name}ModuleTypeId = "{${Random_Uuid}}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* ${Name}EditorModuleTypeId = ${Name}ModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* ${Name}RequestsTypeId = "{${Random_Uuid}}";
} // namespace ${Name}
