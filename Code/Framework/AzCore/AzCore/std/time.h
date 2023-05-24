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
    //! Returns the cpu time spent on the current thread when invoked
    //! This time is relative to the running process and has no relation to real time
    AZStd::chrono::microseconds GetCpuThreadTimeNowMicrosecond();
}
