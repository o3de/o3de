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

namespace AZ::Date::Internal
{
    //! Splits a utc time point into a year-month-day and hour-minutes-seconds structure
    //! The split structures can then be used to format output based on the time point
    //! Returns true if utcTime represents a valid date and time
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

namespace AZ::Date
{
    //! Contains methods to convert a chrono UTC clock time point into
    //! ISO8601 timestamp per https://www.iso.org/iso-8601-date-and-time-format.html
    //!
    //! Alsosee the Wikipedia page on the differences between the Extended format and Basic format
    //! https://en.wikipedia.org/wiki/ISO_8601#Calendar_dates

    //! Type alias use to store a formated ISO8601 timestamp string or a timestamp string that is close to it
    using Iso8601TimestampString = AZStd::fixed_string<64>;

    //! Date Formats
    //! formats an extended format ISO 8601 timestamp date
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601ExtendedFormatDate(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        using DurationType = typename AZStd::remove_cvref_t<decltype(utcTime)>::duration;

        AZStd::chrono::year_month_day currentYmd;
        [[maybe_unused]] AZStd::chrono::hh_mm_ss<DurationType> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = Iso8601TimestampString::format(
            "%.4i-%.2u-%.2u",
            int{ currentYmd.year() },
            unsigned{ currentYmd.month() },
            unsigned{ currentYmd.day() });

            return true;
    }

    //! formats a basic format ISO 8601 timestamp date
    //! This is suitable for filenames
    //! It should be avoid in plain text per the ISO 8601 standard
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601BasicFormatDate(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        using DurationType = typename AZStd::remove_cvref_t<decltype(utcTime)>::duration;

        AZStd::chrono::year_month_day currentYmd;
        [[maybe_unused]] AZStd::chrono::hh_mm_ss<DurationType> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = Iso8601TimestampString::format(
            "%.4i%.2u%.2u",
            int{ currentYmd.year() },
            unsigned{ currentYmd.month() },
            unsigned{ currentYmd.day() });

            return true;
    }

    // Time formats

    //! formats an extended format ISO 8601 timestamp time to seconds
    //! This format is not suitable for filenames as it contains the colon character, which isn't allowed
    //! for windows filesystems(NTFS)
    //! Returns true if utcTime is valid
    //! Use the basic format for filesystem safe timestamp
    template<class Duration>
    bool GetIso8601ExtendedFormatTime(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        using DurationType = typename AZStd::remove_cvref_t<decltype(utcTime)>::duration;

        [[maybe_unused]] AZStd::chrono::year_month_day currentYmd;
        AZStd::chrono::hh_mm_ss<DurationType> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = Iso8601TimestampString::format(
            "T%.2u:%.2u:%.2uZ",
            static_cast<unsigned>(currentTimeOfDay.hours().count()),
            static_cast<unsigned>(currentTimeOfDay.minutes().count()),
            static_cast<unsigned>(currentTimeOfDay.seconds().count()));

        return true;
    }

    //! formats an extended format ISO 8601 timestamp time to milliseconds
    //! This format is not suitable for filenames as it contains the colon character, which isn't allowed
    //! for windows filesystems(NTFS)
    //! Use the basic format for filesystem safe timestamp
    template<class Duration>
    bool GetIso8601ExtendedFormatTimeWithMilliseconds(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        using DurationType = typename AZStd::remove_cvref_t<decltype(utcTime)>::duration;

        [[maybe_unused]] AZStd::chrono::year_month_day currentYmd;
        AZStd::chrono::hh_mm_ss<DurationType> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = Iso8601TimestampString::format(
            "T%.2u:%.2u:%.2u.%.3uZ",
            static_cast<unsigned>(currentTimeOfDay.hours().count()),
            static_cast<unsigned>(currentTimeOfDay.minutes().count()),
            static_cast<unsigned>(currentTimeOfDay.seconds().count()),
            static_cast<unsigned>(AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(currentTimeOfDay.subseconds()).count()));

        return true;
    }

    //! formats an extended format ISO 8601 timestamp time to microseconds
    //! This format is not suitable for filenames as it contains the colon character, which isn't allowed
    //! for windows filesystems(NTFS)
    //! Use the basic format for filesystem safe timestamp
    template<class Duration>
    bool GetIso8601ExtendedFormatTimeWithMicroseconds(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        using DurationType = typename AZStd::remove_cvref_t<decltype(utcTime)>::duration;

        [[maybe_unused]] AZStd::chrono::year_month_day currentYmd;
        AZStd::chrono::hh_mm_ss<DurationType> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = Iso8601TimestampString::format(
            "T%.2u:%.2u:%.2u.%.6uZ",
            static_cast<unsigned>(currentTimeOfDay.hours().count()),
            static_cast<unsigned>(currentTimeOfDay.minutes().count()),
            static_cast<unsigned>(currentTimeOfDay.seconds().count()),
            static_cast<unsigned>(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(currentTimeOfDay.subseconds()).count()));

        return true;
    }

    //! formats a basic format ISO 8601 timestamp time
    //! This is suitable for filenames
    //! It should be avoid in plain text per the ISO 8601 standard
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601BasicFormatTime(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        using DurationType = typename AZStd::remove_cvref_t<decltype(utcTime)>::duration;

        [[maybe_unused]] AZStd::chrono::year_month_day currentYmd;
        AZStd::chrono::hh_mm_ss<DurationType> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = Iso8601TimestampString::format(
            "T%.2u%.2u%.2uZ",
            static_cast<unsigned>(currentTimeOfDay.hours().count()),
            static_cast<unsigned>(currentTimeOfDay.minutes().count()),
            static_cast<unsigned>(currentTimeOfDay.seconds().count()));

        return true;
    }

    //! formats a basic format ISO 8601 timestamp time which includes milliseconds
    //! This is suitable for filenames
    //! It should be avoid in plain text per the ISO 8601 standard
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601BasicFormatTimeWithMilliseconds(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        using DurationType = typename AZStd::remove_cvref_t<decltype(utcTime)>::duration;

        [[maybe_unused]] AZStd::chrono::year_month_day currentYmd;
        AZStd::chrono::hh_mm_ss<DurationType> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = Iso8601TimestampString::format(
            "T%.2u%.2u%.2u.%.3uZ",
            static_cast<unsigned>(currentTimeOfDay.hours().count()),
            static_cast<unsigned>(currentTimeOfDay.minutes().count()),
            static_cast<unsigned>(currentTimeOfDay.seconds().count()),
            static_cast<unsigned>(AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(currentTimeOfDay.subseconds()).count()));

        return true;
    }

    //! formats a basic format ISO 8601 timestamp time which includes microseconds
    //! This is suitable for filenames
    //! It should be avoid in plain text per the ISO 8601 standard
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601BasicFormatTimeWithMicroseconds(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        using DurationType = typename AZStd::remove_cvref_t<decltype(utcTime)>::duration;

        [[maybe_unused]] AZStd::chrono::year_month_day currentYmd;
        AZStd::chrono::hh_mm_ss<DurationType> currentTimeOfDay;
        if (!Internal::GetYearMonthDayWithHoursMinutesSecondsFromUTCTime(currentYmd, currentTimeOfDay, utcTime))
        {
            return false;
        }

        // Print the current utc timestamp
        // using the format specified at https://www.w3.org/TR/NOTE-datetime
        utcTimestamp = Iso8601TimestampString::format(
            "T%.2u%.2u%.2u.%.6uZ",
            static_cast<unsigned>(currentTimeOfDay.hours().count()),
            static_cast<unsigned>(currentTimeOfDay.minutes().count()),
            static_cast<unsigned>(currentTimeOfDay.seconds().count()),
            static_cast<unsigned>(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(currentTimeOfDay.subseconds()).count()));

        return true;
    }


    //! formats an extended format ISO 8601 timestamp with up to seconds
    //! This is not suitable for using as part of a filename as it contains the colon(':') character
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601ExtendedFormat(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        if (Iso8601TimestampString timestampDate; GetIso8601ExtendedFormatDate(timestampDate, utcTime))
        {
            if (Iso8601TimestampString timestampTime; GetIso8601ExtendedFormatTime(timestampTime, utcTime))
            {
                utcTimestamp = timestampDate + timestampTime;
                return true;
            }
        }

        return false;
    }

    //! formats an extended format ISO 8601 timestamp with fractional milliseconds
    //! This is not suitable for using as part of a filename as it contains the colon(':') character
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601ExtendedFormatWithMilliseconds(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        if (Iso8601TimestampString timestampDate; GetIso8601ExtendedFormatDate(timestampDate, utcTime))
        {
            if (Iso8601TimestampString timestampTime; GetIso8601ExtendedFormatTimeWithMilliseconds(timestampTime, utcTime))
            {
                utcTimestamp = timestampDate + timestampTime;
                return true;
            }
        }

        return false;
    }

    //! formats an extended format ISO 8601 timestamp in microseconds
    //! This is not suitable for using as part of a filename as it contains the colon(':') character
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601ExtendedFormatWithMicroseconds(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        if (Iso8601TimestampString timestampDate; GetIso8601ExtendedFormatDate(timestampDate, utcTime))
        {
            if (Iso8601TimestampString timestampTime; GetIso8601ExtendedFormatTimeWithMicroseconds(timestampTime, utcTime))
            {
                utcTimestamp = timestampDate + timestampTime;
                return true;
            }
        }

        return false;
    }

    //! formats a basic format ISO 8601 timestamp with up to seconds
    //! This is suitable for filenames
    //! It should be avoid in plain text per the ISO 8601 standard
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601BasicFormat(Iso8601TimestampString& utcTimestamp, const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        if (Iso8601TimestampString timestampDate; GetIso8601BasicFormatDate(timestampDate, utcTime))
        {
            if (Iso8601TimestampString timestampTime; GetIso8601BasicFormatTime(timestampTime, utcTime))
            {
                utcTimestamp = timestampDate + timestampTime;
                return true;
            }
        }

        return false;
    }

    //! formats a basic format ISO 8601 timestamp with fractional milliseconds
    //! This is suitable for filenames
    //! It should be avoid in plain text per the ISO 8601 standard
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601BasicFormatWithMilliseconds(Iso8601TimestampString& utcTimestamp,
        const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        if (Iso8601TimestampString timestampDate; GetIso8601BasicFormatDate(timestampDate, utcTime))
        {
            if (Iso8601TimestampString timestampTime; GetIso8601BasicFormatTimeWithMilliseconds(timestampTime, utcTime))
            {
                utcTimestamp = timestampDate + timestampTime;
                return true;
            }
        }

        return false;
    }

    //! formats a basic format ISO 8601 timestamp in microseconds
    //! This is suitable for filenames
    //! It should be avoid in plain text per the ISO 8601 standard
    //! Returns true if utcTime is valid
    template<class Duration>
    bool GetIso8601BasicFormatWithMicroseconds(Iso8601TimestampString& utcTimestamp,
        const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        if (Iso8601TimestampString timestampDate; GetIso8601BasicFormatDate(timestampDate, utcTime))
        {
            if (Iso8601TimestampString timestampTime; GetIso8601BasicFormatTimeWithMicroseconds(timestampTime, utcTime))
            {
                utcTimestamp = timestampDate + timestampTime;
                return true;
            }
        }

        return false;
    }

    //! formats a ISO 8601 like timestamp that is safe to use as a filename
    //! It combines the Extended ISO 8601 Date + Basic ISO 8601 Time
    template<class Duration>
    bool GetFilenameCompatibleFormat(Iso8601TimestampString& utcTimestamp,
        const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        if (Iso8601TimestampString timestampExtendedDate;
            GetIso8601ExtendedFormatDate(timestampExtendedDate, utcTime))
        {
            if (Iso8601TimestampString timestampBasicTime;
                GetIso8601BasicFormatTime(timestampBasicTime, utcTime))
            {
                utcTimestamp = timestampExtendedDate + timestampBasicTime;
                return true;
            }
        }

        return false;
    }

    //! formats a ISO 8601 like timestamp that is safe to use as a filename
    //! It combines the Extended ISO 8601 Date + Basic ISO 8601 Time
    //! This includes milliseconds as part of the timestamp
    template<class Duration>
    bool GetFilenameCompatibleFormatWithMilliseconds(Iso8601TimestampString& utcTimestamp,
        const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        if (Iso8601TimestampString timestampExtendedDate;
            GetIso8601ExtendedFormatDate(timestampExtendedDate, utcTime))
        {
            if (Iso8601TimestampString timestampBasicTime;
                GetIso8601BasicFormatTimeWithMilliseconds(timestampBasicTime, utcTime))
            {
                utcTimestamp = timestampExtendedDate + timestampBasicTime;
                return true;
            }
        }

        return false;
    }

    //! formats a ISO 8601 like timestamp that is safe to use as a filename
    //! It combines the Extended ISO 8601 Date + Basic ISO 8601 Time
    //! This includes microseconds as part of the timestamp
    template<class Duration>
    bool GetFilenameCompatibleFormatWithMicroseconds(Iso8601TimestampString& utcTimestamp,
        const AZStd::chrono::utc_time<Duration>& utcTime)
    {
        if (Iso8601TimestampString timestampExtendedDate;
            GetIso8601ExtendedFormatDate(timestampExtendedDate, utcTime))
        {
            if (Iso8601TimestampString timestampBasicTime;
                GetIso8601BasicFormatTimeWithMicroseconds(timestampBasicTime, utcTime))
            {
                utcTimestamp = timestampExtendedDate + timestampBasicTime;
                return true;
            }
        }

        return false;
    }

    // The GetIso8601*Now versions queries the current utc time and formats it into a fixed string
    //! returns ISO 8601 extended format date + time based on the current time
    //! The timestamp will be in the format of "YYYY-MM-DD[T]HH:MM:SS[Z]"
    //! Ex. "2025-04-21T13:17:55"
    bool GetIso8601ExtendedFormatNow(Iso8601TimestampString& utcTimestamp);

    //! returns ISO 8601 extended format date + time based on the current time with fractional milliseconds included
    //! The timestamp will be in the format of "YYYY-MM-DD[T]HH:MM:SS.fff[Z]"
    //! Ex. "2025-04-21T13:17:55.537Z"
    bool GetIso8601ExtendedFormatNowWithMilliseconds(Iso8601TimestampString& utcTimestamp);

    //! returns ISO 8601 extended format date + timebased on the current time with fractional microseconds included
    //! The timestamp will be in the format of "YYYY-MM-DD[T]HH:MM:SS.ffffff[Z]"
    //! Ex. "2025-04-21T13:17:55.537982Z"
    bool GetIso8601ExtendedFormatNowWithMicroseconds(Iso8601TimestampString& utcTimestamp);

    //! returns ISO 8601 basic format date + timebased on the current time
    //! The timestamp will be in the format of "YYYYMMDD[T]HHMMSS[Z]"
    //! Ex. "20250421T131755537982Z"
    bool GetIso8601BasicFormatNow(Iso8601TimestampString& utcTimestamp);

    //! returns ISO 8601 extended format date + timebased on the current time with fractional milliseconds included
    //! The timestamp will be in the format of "YYYY-MM-DD[T]HH:MM:SS.fff[Z]"
    //! Ex. 20250421T131755.537Z
    bool GetIso8601BasicFormatNowWithMilliseconds(Iso8601TimestampString& utcTimestamp);

    //! returns ISO 8601 extended format date + timebased on the current time with fractional microseconds included
    //! The timestamp will be in the format of "YYYYMMDD[T]HHMMSS.ffffff[Z]"
    //! Ex. "20250421T131755.537982Z"
    bool GetIso8601BasicFormatNowWithMicroseconds(Iso8601TimestampString& utcTimestamp);

    //! returns a non-standard ISO 8601 style timestampe which is safe to use as a filename on Windows and Posix platforms
    //! It combines the extended format for the date portion with the basic format for the time portion
    //! The timestamp will be in the format of "YYYY-MM-DD[T]HHMMSS[Z]"
    //! Ex. "2025-04-21T131755Z"
    bool GetFilenameCompatibleFormatNow(Iso8601TimestampString& utcTimestamp);

    //! returns a non-standard ISO 8601 style timestampe which is safe to use as a filename on Windows and Posix platforms
    //! It combines the extended format for the date portion with the basic format for the time portion
    //! The timestamp includes fractional milliseconds and it will be in the format of "YYYY-MM-DD[T]HHMMSS.fff[Z]"
    //! Ex. "2025-04-21T131755.537Z"
    bool GetFilenameCompatibleFormatNowWithMilliseconds(Iso8601TimestampString& utcTimestamp);

    //! returns a non-standard ISO 8601 style timestampe which is safe to use as a filename on Windows and Posix platforms
    //! It combines the extended format for the date portion with the basic format for the time portion
    //! The timestamp includes fractional microseconds and it will be in the format of "YYYY-MM-DD[T]HHMMSS.ffffff[Z]"
    //! Ex. "2025-04-21T131755.537982Z"
    bool GetFilenameCompatibleFormatNowWithMicroseconds(Iso8601TimestampString& utcTimestamp);
}
