/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Device_Windows.h>

namespace AZ::Vulkan::Platform
{
    uint32_t GetMainDisplayRefreshRateInternal() 
    {
        DEVMODE DisplayConfig;
        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DisplayConfig);
        return DisplayConfig.dmDisplayFrequency;
    }
} 

