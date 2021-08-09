/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Device_Mac.h>

namespace AZ
{
    namespace Metal
    {
        namespace Platform
        {
            float GetMainDisplayRefreshRateInternal()
            {
                CGDirectDisplayID display = CGMainDisplayID();
                CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(display);
                return CGDisplayModeGetRefreshRate(currentMode);
            }
        } // namespace Platform
    } // namespace Metal
} // namespace AZ
