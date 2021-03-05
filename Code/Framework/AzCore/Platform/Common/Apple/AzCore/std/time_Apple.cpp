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
        struct timespec ts;
        clock_serv_t cclock;
        mach_timespec_t mts;
        kern_return_t ret = host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        if (ret == KERN_SUCCESS)
        {
            ret = clock_get_time(cclock, &mts);
            if (ret == KERN_SUCCESS)
            {
                ts.tv_sec = mts.tv_sec;
                ts.tv_nsec = mts.tv_nsec;
            }
            else
            {
                AZ_Assert(false, "clock_get_time error: %d\n", ret);
            }
            mach_port_deallocate(mach_task_self(), cclock);
        }
        else
        {
            AZ_Assert(false, "clock_get_time error: %d\n", ret);
        }
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
        clock_serv_t cclock;
        mach_timespec_t mts;
        kern_return_t ret = host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        if (ret == KERN_SUCCESS)
        {
            ret = clock_get_time(cclock, &mts);
            if (ret == KERN_SUCCESS)
            {
                ts.tv_sec = mts.tv_sec;
                ts.tv_nsec = mts.tv_nsec;
            }
            else
            {
                AZ_Assert(false, "clock_get_time error: %d\n", ret);
            }
            mach_port_deallocate(mach_task_self(), cclock);
        }
        else
        {
            AZ_Assert(false, "clock_get_time error: %d\n", ret);
        }
        timeNowSecond =  ts.tv_sec;
        return timeNowSecond;
    }

    AZ::u64 GetTimeUTCMilliSecond()
    {
        AZ::u64 utc;
        struct timespec ts;
        clock_serv_t cclock;
        mach_timespec_t mts;
        kern_return_t ret = host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        if (ret == KERN_SUCCESS)
        {
            ret = clock_get_time(cclock, &mts);
            if (ret == KERN_SUCCESS)
            {
                ts.tv_sec = mts.tv_sec;
                ts.tv_nsec = mts.tv_nsec;
            }
            else
            {
                AZ_Assert(false, "clock_get_time error: %d\n", ret);
            }
            mach_port_deallocate(mach_task_self(), cclock);
        }
        else
        {
            AZ_Assert(false, "clock_get_time error: %d\n", ret);
        }
        utc =  ts.tv_sec * 1000L +  ts.tv_nsec / 1000000L;
        return utc;
    }
}
