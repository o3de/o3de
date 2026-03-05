/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array.h>
#include <AzCore/std/math.h>
#include <AzCore/std/metaprogramming/ratio.h>
#include <AzCore/std/typetraits/is_integral.h>

#include <chrono>

namespace AZStd::chrono
{
    using std::chrono::duration;
    using std::chrono::time_point;

    using std::chrono::duration_values;
    using std::chrono::treat_as_floating_point;
    using std::chrono::treat_as_floating_point_v;
    using std::chrono::is_clock;
    using std::chrono::is_clock_v;

    using std::chrono::ceil;
    using std::chrono::duration_cast;
    using std::chrono::time_point_cast;
    using std::chrono::floor;
    using std::chrono::round;

    using std::chrono::hours;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::minutes;
    using std::chrono::nanoseconds;
    using std::chrono::seconds;
    using std::chrono::days;
    using std::chrono::months;
    using std::chrono::weeks;
    using std::chrono::years;

    using std::chrono::abs;

    using std::chrono::system_clock;
    using std::chrono::sys_days;
    using std::chrono::sys_seconds;
    using std::chrono::sys_time;

    using std::chrono::utc_clock;
    using std::chrono::utc_seconds;
    using std::chrono::utc_time;
    using std::chrono::get_leap_second_info;
    using std::chrono::leap_second;
    using std::chrono::leap_second_info;

    using std::chrono::tai_clock;
    using std::chrono::tai_seconds;
    using std::chrono::tai_time;

    using std::chrono::gps_clock;
    using std::chrono::gps_seconds;
    using std::chrono::gps_time;

    using std::chrono::file_clock;
    using std::chrono::file_time;

    using std::chrono::steady_clock;

    using std::chrono::high_resolution_clock;

    using std::chrono::local_days;
    using std::chrono::local_seconds;
    using std::chrono::local_t;
    using std::chrono::local_time;

    using std::chrono::clock_cast;
    using std::chrono::clock_time_conversion;

    using std::chrono::last_spec;
    using std::chrono::day;
    using std::chrono::month;
    using std::chrono::year;
    using std::chrono::weekday;
    using std::chrono::weekday_indexed;
    using std::chrono::weekday_last;
    using std::chrono::month_day;
    using std::chrono::month_day_last;
    using std::chrono::month_weekday;
    using std::chrono::month_weekday_last;
    using std::chrono::year_month;
    using std::chrono::year_month_day;
    using std::chrono::year_month_day_last;
    using std::chrono::year_month_weekday;
    using std::chrono::year_month_weekday_last;

    using std::chrono::hh_mm_ss;

    using std::chrono::is_am;
    using std::chrono::is_pm;
    using std::chrono::make12;
    using std::chrono::make24;

    using std::chrono::last;

    using std::chrono::Friday;
    using std::chrono::Monday;
    using std::chrono::Saturday;
    using std::chrono::Sunday;
    using std::chrono::Thursday;
    using std::chrono::Tuesday;
    using std::chrono::Wednesday;

    using std::chrono::April;
    using std::chrono::August;
    using std::chrono::December;
    using std::chrono::February;
    using std::chrono::January;
    using std::chrono::July;
    using std::chrono::June;
    using std::chrono::March;
    using std::chrono::May;
    using std::chrono::November;
    using std::chrono::October;
    using std::chrono::September;

    using std::chrono::tzdb;
    using std::chrono::tzdb_list;
    using std::chrono::get_tzdb;
    using std::chrono::get_tzdb_list;
    using std::chrono::reload_tzdb;
    using std::chrono::remote_version;
    using std::chrono::locate_zone;
    using std::chrono::current_zone;

    using std::chrono::time_zone;

    using std::chrono::zoned_traits;
    using std::chrono::zoned_time;
    using std::chrono::zoned_seconds;

    using std::chrono::time_zone_link;

    using std::chrono::ambiguous_local_time;
    using std::chrono::nonexistent_local_time;

    using std::chrono::choose;
} // namespace AZStd::chrono

namespace AZStd
{
    inline namespace literals
    {
        inline namespace chrono_literals
        {
            using namespace std::literals::chrono_literals;

            // [time.cal.day.nonmembers](http://eel.is/c++draft/time#cal.day.nonmembers), non-member functions
            constexpr chrono::day operator""_d(unsigned long long d) noexcept
            {
                return chrono::day{static_cast<unsigned int>(d)};
            }

            // [time.cal.year.nonmembers](http://eel.is/c++draft/time#cal.year.nonmembers), non-member functions
            constexpr chrono::year operator""_y(unsigned long long y) noexcept
            {
                return chrono::year{static_cast<int>(y)};
            }
        }
    } // namespace literals

    namespace chrono
    {
        using namespace literals::chrono_literals;
    }
} // namespace AZStd
