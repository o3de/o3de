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
        return GetIso8601ExtendedFormat(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetIso8601ExtendedFormatNowWithMilliseconds(Iso8601TimestampString& utcTimestamp)
    {
        return GetIso8601ExtendedFormatTimeWithMilliseconds(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetIso8601ExtendedFormatNowWithMicroseconds(Iso8601TimestampString& utcTimestamp)
    {
        return GetIso8601ExtendedFormatTimeWithMicroseconds(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetIso8601BasicFormatNow(Iso8601TimestampString& utcTimestamp)
    {
        return GetIso8601BasicFormat(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetIso8601BasicFormatNowWithMilliseconds(Iso8601TimestampString& utcTimestamp)
    {
        return GetIso8601BasicFormatTimeWithMilliseconds(utcTimestamp, AZStd::chrono::utc_clock::now());
    }

    bool GetIso8601BasicFormatNowWithMicroseconds(Iso8601TimestampString& utcTimestamp)
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
