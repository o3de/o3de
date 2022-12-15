/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/time.h>
#include <AzCore/PlatformIncl.h>

namespace AZStd
{
    AZStd::sys_time_t GetTimeTicksPerSecond()
    {
        static AZStd::sys_time_t freq = 0;
        if (freq == 0)
        {
            QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        }
        return freq;
    }

    AZStd::sys_time_t GetTimeNowTicks()
    {
        AZStd::sys_time_t timeNow;
        QueryPerformanceCounter((LARGE_INTEGER*)&timeNow);
        return timeNow;
    }

    AZStd::sys_time_t GetTimeNowMicroSecond()
    {
        AZStd::sys_time_t timeNowMicroSecond;
        // NOTE: The default line below was not working on systems with smaller TicksPerSecond() values (like in Windows7, for example)
        // So we are now spreading the division between the numerator and denominator to maintain better precision
        timeNowMicroSecond = (GetTimeNowTicks() * 1000) / (GetTimeTicksPerSecond() / 1000);
        return timeNowMicroSecond;
    }

    AZStd::sys_time_t GetTimeNowSecond()
    {
        AZStd::sys_time_t timeNowSecond;
        // Using get tick count, since it's more stable for longer time measurements.
        timeNowSecond = GetTickCount() / 1000;
        return timeNowSecond;
    }

    AZStd::chrono::microseconds GetCpuThreadTimeNowMicrosecond()
    {
        auto ComputeTimestampCounterPerMicrosecond = []() -> int64_t
        {
            static int64_t timeStampCounterPerMicrosecond = 0;
            if (timeStampCounterPerMicrosecond > 0)
            {
                return timeStampCounterPerMicrosecond;
            }

            // To measure the Cpu time with microsecond precision
            // on Windows Intel CPUs, the QueryThreadCycleTime is being used
            // despite the documentation indicating that this function
            // shouldn't be used to convert to elapsed time
            // https://learn.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-querythreadcycletime#remarks

            // What is being done here is to measure the processor timestamp difference
            // over a period of 50 milliseconds and use that as the rate at which the thread ticks per microsecond
            static const unsigned long long initialProcessorTimestamp = __rdtsc();
            static const AZStd::chrono::steady_clock::time_point initialTime = AZStd::chrono::steady_clock::now();
            const unsigned long long processorTimestamp = __rdtsc();
            const auto elapsedTimeSinceFirstCall =
                AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - initialTime);

            // Wait 50 milliseconds after the first call to this function
            // to measure the thread difference in the time-stamp counter for the processor
            if (elapsedTimeSinceFirstCall <= AZStd::chrono::milliseconds(50))
            {
                // return 0 until more accurate data can be gathered
                return 0;
            }

            // elapsedTimeSinceFirstCall is in microseconds
            timeStampCounterPerMicrosecond = static_cast<int64_t>((processorTimestamp - initialProcessorTimestamp) / elapsedTimeSinceFirstCall.count());
            return timeStampCounterPerMicrosecond;
        };

        if (int64_t timeStampCounterPerMicrosecond = ComputeTimestampCounterPerMicrosecond();
            timeStampCounterPerMicrosecond > 0)
        {
            unsigned long long threadCycleCount;
            BOOL result = ::QueryThreadCycleTime(GetCurrentThread(), &threadCycleCount);
            if (!result)
            {
                auto FormatFunc = [](wchar_t* buffer, size_t size)
                {
                    return ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, GetLastError(), 0, buffer, static_cast<DWORD>(size),
                        nullptr);
                };
                constexpr size_t messageBufferSize = 1024;
                AZStd::fixed_wstring<messageBufferSize> errorMessage;
                errorMessage.resize_and_overwrite(errorMessage.capacity(), AZStd::move(FormatFunc));
                AZ_Assert(false, "Failed to query thread cycle time through QueryThreadCycleTime: "
                    AZ_TRAIT_FORMAT_STRING_PRINTF_WSTRING "\n", errorMessage.c_str());
                return AZStd::chrono::microseconds::zero();
            }
            return AZStd::chrono::microseconds(threadCycleCount / timeStampCounterPerMicrosecond);
        }

        // return 0 until more accurate data can be gathered
        return AZStd::chrono::microseconds::zero();
    }
}
