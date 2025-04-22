/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/time.h>

namespace AZStd
{
    // return time in millisecond since 1970/01/01 00:00:00 UTC.
    AZ::u64 GetTimeUTCMilliSecond()
    {
        return AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::utc_clock::now().time_since_epoch()).count();
    }
    AZ::u64 GetTimeUTCMicroSecond()
    {
        return AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::utc_clock::now().time_since_epoch()).count();
    }
}
