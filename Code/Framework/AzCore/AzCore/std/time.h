/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZStd
{
    AZStd::sys_time_t GetTimeTicksPerSecond();
    AZStd::sys_time_t GetTimeNowTicks();
    AZStd::sys_time_t GetTimeNowMicroSecond();
    AZStd::sys_time_t GetTimeNowSecond();
    // return time in millisecond since 1970/01/01 00:00:00 UTC.
    AZ::u64           GetTimeUTCMilliSecond();
    AZ::u64           GetTimeUTCMicroSecond();

    // Returns true if utcTime is valid

    namespace Internal
    {
        template<class Duration, class HH_MM_SS>
        bool GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(AZStd::chrono::year_month_day& yearMonthDay, HH_MM_SS& hoursMinutesSeconds,
            const AZStd::chrono::utc_time<Duration>& utcTime)
        {
            // Compute year-month-day by getting days since the epoch 1970-01-01 00:00:00 UTC
            // using the utc_clock which accounts for leap seconds

            // Year-month-day is only computable from a system_clock time_point
            // Convert the UTC time to system_clock::time_point
            const auto utcToSysTime = AZStd::chrono::utc_clock::to_sys(utcTime);
            const auto serialUtcDays = AZStd::chrono::time_point_cast<AZStd::chrono::days>(utcToSysTime);

            const AZStd::chrono::year_month_day currentYmd{ serialUtcDays };
            // Validate that the date is valid in the civil calendar
            if (!currentYmd.ok())
            {
                return false;
            }

            // Compute a hour-minute-second(subsecond) structure from the difference in the utc time since the epoch
            // and days since the epoch
            // Computes the duration since the epoch representing the start of the day
            const HH_MM_SS currentTimeOfDay{ utcToSysTime - serialUtcDays };
            // The time of day should never be negative
            if (currentTimeOfDay.is_negative())
            {
                return false;
            }

            // Update the output parameters and return success
            yearMonthDay = currentYmd;
            hoursMinutesSeconds = currentTimeOfDay;
            return true;

        }
    }

    using UTCTimestampString = AZStd::fixed_string<64>;

    // formats a UTC timestamp with up to seconds
    // Returns true if utcTime is valid
    template<class Duration>
    bool GetUTCTimestampSeconds(UTCTimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        AZStd::chrono::year_month_day currentYmd;
        AZStd::chrono::hh_mm_ss<typename AZStd::remove_cvref_t<decltype(utcTime)>::duration> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = UTCTimestampString::format(
            "The current UTC time is %.4i-%.2u-%.2uT%.2u:%.2u:%.2uZ\n",
            int{ currentYmd.year() },
            unsigned{ currentYmd.month() },
            unsigned{ currentYmd.day() },
            static_cast<unsigned>(currentTimeOfDay.hours().count()),
            static_cast<unsigned>(currentTimeOfDay.minutes().count()),
            static_cast<unsigned>(currentTimeOfDay.seconds().count()));

        return true;
    }

    // formats a UTC timestamp with fractional millseconds
    // Returns true if utcTime is valid
    template<class Duration>
    bool GetUTCTimestampMilliseconds(UTCTimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        AZStd::chrono::year_month_day currentYmd;
        AZStd::chrono::hh_mm_ss<typename AZStd::remove_cvref_t<decltype(utcTime)>::duration> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp with fractional seconds up to milliseconds
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = UTCTimestampString::format(
            "The current UTC time is %.4i-%.2u-%.2uT%.2u:%.2u:%.2u.%.3uZ\n",
            int{ currentYmd.year() },
            unsigned{ currentYmd.month() },
            unsigned{ currentYmd.day() },
            static_cast<unsigned>(currentTimeOfDay.hours().count()),
            static_cast<unsigned>(currentTimeOfDay.minutes().count()),
            static_cast<unsigned>(currentTimeOfDay.seconds().count()),
            static_cast<unsigned>(currentTimeOfDay.subseconds().count()));

        return true;
    }

    // formats a UTC timestamp in microseconds
    // Returns true if utcTime is valid
    template<class Duration>
    bool GetUTCTimestampMicroseconds(UTCTimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        AZStd::chrono::year_month_day currentYmd;
        AZStd::chrono::hh_mm_ss<typename AZStd::remove_cvref_t<decltype(utcTime)>::duration> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp with fractional seconds up to microseconds
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = UTCTimestampString::format(
            "The current UTC time is %.4i-%.2u-%.2uT%.2u:%.2u:%.2u.%.6uZ\n",
            int{ currentYmd.year() },
            unsigned{ currentYmd.month() },
            unsigned{ currentYmd.day() },
            static_cast<unsigned>(currentTimeOfDay.hours().count()),
            static_cast<unsigned>(currentTimeOfDay.minutes().count()),
            static_cast<unsigned>(currentTimeOfDay.seconds().count()),
            static_cast<unsigned>(currentTimeOfDay.subseconds().count()));

        return true;
    }

    // The GetUTCTimeStamp*Now versions queries the current utc time and formats it into a fixed string
    bool GetUTCTimestampSecondsNow(UTCTimestampString& utcTimestamp);
    bool GetUTCTimestampMillisecondsNow(UTCTimestampString& utcTimestamp);
    bool GetUTCTimestampMicrosecondsNow(UTCTimestampString& utcTimestamp);
}
