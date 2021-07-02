/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Crash Handler shared support

#include <CrashSupport.h>

#include <time.h>

namespace CrashHandler
{
    std::string GetTimeString()
    {
        const int bufLen = 100;
        char buffer[bufLen];

        struct tm timeinfo;

        GetTimeInfo(timeinfo);

        strftime(buffer, bufLen, "%Y-%m-%dT%H:%M:%S", &timeinfo);
        return buffer;
    }

}
