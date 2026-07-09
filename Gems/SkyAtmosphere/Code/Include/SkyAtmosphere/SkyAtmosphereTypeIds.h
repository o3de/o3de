/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace SkyAtmosphere
{
    // System Component TypeIds
    inline constexpr const char* SkyAtmosphereSystemComponentTypeId = "{C674D50A-5DDF-49EF-9D6D-21C114238DD8}";
    inline constexpr const char* SkyAtmosphereEditorSystemComponentTypeId = "{3771193D-2A44-42EF-8387-4E99CF777A65}";

    // Module derived classes TypeIds
    inline constexpr const char* SkyAtmosphereModuleInterfaceTypeId = "{62965C3F-55CB-42F2-A1AA-41946C5E05E1}";
    inline constexpr const char* SkyAtmosphereModuleTypeId = "{4430CB77-B63A-4625-91F6-2E65DF07BED3}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* SkyAtmosphereEditorModuleTypeId = SkyAtmosphereModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* SkyAtmosphereRequestsTypeId = "{BA2A41F1-ED1A-40F8-A4C3-60D98A1557B9}";
} // namespace SkyAtmosphere
