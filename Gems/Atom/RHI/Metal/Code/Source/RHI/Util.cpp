/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <mach/mach_time.h>

namespace AZ
{
    namespace Metal
    {
        double timebaseScale = 0.0;
        double GetWallTimeSinceApplicationStart()
        {
            if (timebaseScale == 0.0)
            {
                static mach_timebase_info_data_t timebase = {0, 0};
                mach_timebase_info(&timebase);
                timebaseScale = static_cast<double>(timebase.numer) / (1e9 * static_cast<double>(timebase.denom));
            }
            return static_cast<double>(mach_absolute_time()) * timebaseScale;
        }

    }
}
