/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Device_Linux.h>

namespace AZ::Vulkan::Platform
{
    uint32_t GetMainDisplayRefreshRateInternal() 
    {
        // [GFX TODO][ATOM-16236] - Add support to query the refresh rate
        return 60;
    }
} 

