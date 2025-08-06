/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AFR
{
    // System Component TypeIds
    inline constexpr const char* AFRSystemComponentTypeId = "{9417D00E-172B-44D9-9945-FFD2FFB6D866}";
    inline constexpr const char* AFREditorSystemComponentTypeId = "{720D600B-ADA3-4E3E-B1FC-316547FDF772}";

    // Module derived classes TypeIds
    inline constexpr const char* AFRModuleInterfaceTypeId = "{E50CB15F-6EDC-4150-98D5-EEAFD88DC348}";
    inline constexpr const char* AFRModuleTypeId = "{C2CEF52D-20F4-4E86-9A9C-56A77032879D}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* AFREditorModuleTypeId = AFRModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* AFRRequestsTypeId = "{38D283F8-6456-40AC-AB51-C5CD04D747D0}";
} // namespace AFR
