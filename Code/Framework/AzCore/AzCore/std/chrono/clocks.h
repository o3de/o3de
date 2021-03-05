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
#ifndef AZSTD_CHRONO_CLOCKS_H
#define AZSTD_CHRONO_CLOCKS_H

#include <AzCore/std/chrono/types.h>
#include <AzCore/std/time.h>

namespace AZStd
{
    /**
     * IMPORTANT: This is not the full standard implementation, it is the same except for supported Rep types.
     * At the moment you can use only types "AZStd::sys_time_t" and "float" as duration Rep parameter. Any other type
     * will lead to either a compiler error or bad results.
     * Everything else should be up to standard. The reason for that is the use of common_type which requires compiler support or rtti.
     * We can't rely on any of those for the variety of compilers we use.
     */
    namespace chrono
    {
        //----------------------------------------------------------------------------//
        //                                                                            //
        //      20.9.5 Clocks [time.clock]                                            //
        //                                                                            //
        //----------------------------------------------------------------------------//

        // If you're porting, clocks are the system-specific (non-portable) part.
        // You'll need to know how to get the current time and implement that under now().
        // You'll need to know what units (tick period) and representation makes the most
        // sense for your clock and set those accordingly.
        // If you know how to map this clock to time_t (perhaps your clock is std::time, which
        // makes that trivial), then you can fill out system_clock's to_time_t() and from_time_t().

        //----------------------------------------------------------------------------//
        //      20.9.5.1 Class system_clock [time.clock.system]                       //
        //----------------------------------------------------------------------------//
        class system_clock
        {
        public:
            typedef microseconds                 duration;
            typedef duration::rep                rep;
            typedef duration::period             period;
            typedef chrono::time_point<system_clock>     time_point;
            static const bool is_monotonic =     true;

            // Get the current time
            static time_point  now()        { return time_point(duration(AZStd::GetTimeNowMicroSecond())); }

            //          static std::time_t to_time_t(const time_point& t);
            //          static time_point  from_time_t(std::time_t t);
        };

        //----------------------------------------------------------------------------//
        //      20.9.5.2 Class monotonic_clock [time.clock.monotonic]                 //
        //----------------------------------------------------------------------------//
        typedef system_clock    monotonic_clock;        // as permitted by [time.clock.monotonic]
        typedef monotonic_clock high_resolution_clock;  // as permitted by [time.clock.hires]
    }
}

#endif // AZSTD_CHRONO_CLOCKS_H
#pragma once
