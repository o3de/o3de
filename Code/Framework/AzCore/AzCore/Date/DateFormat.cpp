/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Date/DateFormat.h>

namespace AZ::Date
{
    bool GetIso8601ExtendedFormatNow(Iso8601TimestampString& utcTimestamp)
    {
        return GetIso8601ExtendedFormatTime(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetIso8601ExtendedFormatMillisecondsNow(Iso8601TimestampString& utcTimestamp)
    {
        return GetIso8601ExtendedFormatTimeWithMilliseconds(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetIso8601ExtendedFormatMicrosecondsNow(Iso8601TimestampString& utcTimestamp)
    {
        return GetIso8601ExtendedFormatTimeWithMicroseconds(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetIso8601BasicFormatNow(Iso8601TimestampString& utcTimestamp)
    {
        return GetIso8601BasicFormatTime(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetIso8601BasicFormatMillisecondsNow(Iso8601TimestampString& utcTimestamp)
    {
        return GetIso8601BasicFormatTimeWithMilliseconds(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetIso8601BasicFormatMicrosecondsNow(Iso8601TimestampString& utcTimestamp)
    {
        return GetIso8601BasicFormatTimeWithMicroseconds(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetFilenameCompatibleFormatNow(Iso8601TimestampString& utcTimestamp)
    {
        return GetFilenameCompatibleFormat(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetFilenameCompatibleFormatNowWithMilliseconds(Iso8601TimestampString& utcTimestamp)
    {
        return GetFilenameCompatibleFormatWithMilliseconds(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetFilenameCompatibleFormatNowWithMicroseconds(Iso8601TimestampString& utcTimestamp)
    {
        return GetFilenameCompatibleFormatWithMicroseconds(utcTimestamp, AZStd::chrono::utc_clock::now());
    }
}
