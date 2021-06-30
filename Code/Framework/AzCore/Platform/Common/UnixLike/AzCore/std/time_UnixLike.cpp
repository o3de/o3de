/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/time.h>
#include <time.h>
#include <errno.h>

namespace AZStd
{
    AZStd::sys_time_t GetTimeTicksPerSecond()
    {
        // Value is in nano seconds, return number of nano seconds in one second
        AZStd::sys_time_t freq = 1000000000L;
        return freq;
    }

    AZStd::sys_time_t GetTimeNowTicks()
    {
        AZStd::sys_time_t timeNow;
        struct timespec ts;
        int result = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);  // Similar to CLOCK_MONOTONIC, but provides access to a raw hardware-based time that is not subject to NTP adjustments.
        if (result < 0)
        {
            // NOTE(HS): WSL doesn't implement support for CLOCK_MONOTONIC_RAW and there
            // is no way to check platform for conditional compile. Fallback to supported
            // option if desired option fails.
            result = clock_gettime(CLOCK_MONOTONIC, &ts);
        }
        AZ_Assert(result != -1, "clock_gettime error: %s\n", strerror(errno));
        timeNow =  ts.tv_sec * GetTimeTicksPerSecond() + ts.tv_nsec;
        return timeNow;
    }

    AZStd::sys_time_t GetTimeNowMicroSecond()
    {
        AZStd::sys_time_t timeNowMicroSecond;
        timeNowMicroSecond =  GetTimeNowTicks() / 1000L;
        return timeNowMicroSecond;
    }

    // time in seconds is the same as the POSIX time_t
    AZStd::sys_time_t GetTimeNowSecond()
    {
        AZStd::sys_time_t timeNowSecond;
        struct timespec ts;
        int result = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);  // Similar to CLOCK_MONOTONIC, but provides access to a raw hardware-based time that is not subject to NTP adjustments.
        (void)result;
        AZ_Assert(result != -1, "clock_gettime error: %s\n", strerror(errno));
        timeNowSecond =  ts.tv_sec;
        return timeNowSecond;
    }

    AZ::u64 GetTimeUTCMilliSecond()
    {
        AZ::u64 utc;
        struct timespec ts;
        int result = clock_gettime(CLOCK_REALTIME, &ts);
        (void)result;
        AZ_Assert(result != -1, "clock_gettime error %s\n", strerror(errno));
        utc =  ts.tv_sec * 1000L +  ts.tv_nsec / 1000000L;
        return utc;
    }
}
