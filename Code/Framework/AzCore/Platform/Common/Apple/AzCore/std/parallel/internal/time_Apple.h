/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <time.h>
#include <errno.h>
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_time.h>

namespace AZStd
{
    namespace Internal
    {
        AZ_FORCE_INLINE timespec CurrentTime()
        {
            struct timespec ts;
            clock_serv_t cclock;
            mach_timespec_t mts;
            host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
            clock_get_time(cclock, &mts);
            mach_port_deallocate(mach_task_self(), cclock);
            ts.tv_sec = mts.tv_sec;
            ts.tv_nsec = mts.tv_nsec;
            return ts;
        }

        template <class Rep, class Period>
        AZ_FORCE_INLINE timespec CurrentTimeAndOffset(const chrono::duration<Rep, Period>& rel_time)
        {
            timespec ts = CurrentTime();

            auto timeInSeconds = chrono::duration_cast<chrono::seconds>(rel_time);
            ts.tv_sec += timeInSeconds.count();
            ts.tv_nsec += chrono::duration_cast<chrono::nanoseconds>(rel_time - timeInSeconds).count();

            // note that ts.tv_nsec could have overflowed 'one second' worth of nanoseconds but since CurrentTime()
            // will always return under 1,000,000,000 nanoseconds and so will the subtraction above here, the
            // largest the two could be is 1,999,999,999 nanoseconds when summed together, so we do not have to do
            // a while loop, one subtraction is enough.
            if (ts.tv_nsec >= 1000000000)
            {
                ts.tv_nsec -= 1000000000;
                ts.tv_sec += 1;
            }

            return ts;
        }
    }
}
