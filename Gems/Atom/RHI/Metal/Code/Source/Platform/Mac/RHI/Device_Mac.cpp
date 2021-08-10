/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Device_Mac.h>

namespace AZ::Metal::Platform
{
    uint32_t GetMainDisplayRefreshRateInternal() const
    {
        CGDirectDisplayID display = CGMainDisplayID();
        CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(display);
        return CGDisplayModeGetRefreshRate(currentMode);
    }
} 

