/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
