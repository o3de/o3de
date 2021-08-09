/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Device_iOS.h>
#include <Atom_RHI_Metal_Platform.h>

namespace AZ
{
    namespace Metal
    {
        namespace Platform
        {
            float GetMainDisplayRefreshRateInternal()
            {
                NativeScreenType* nativeScreen = [NativeScreenType mainScreen];
                return [nativeScreen maximumFramesPerSecond];
            }
        }
    }
}
