/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/time.h>
#include <time.h>
#include <errno.h>
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_time.h>

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
        timeNow =  clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
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
        timeNowSecond =  GetTimeNowTicks()/GetTimeTicksPerSecond();
        return timeNowSecond;
    }

    AZStd::chrono::microseconds GetCpuThreadTimeNowMicrosecond()
    {
        return AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::nanoseconds(clock_gettime_nsec_np(CLOCK_THREAD_CPUTIME_ID)));
    }
}
