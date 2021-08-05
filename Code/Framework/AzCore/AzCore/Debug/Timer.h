/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/time.h>

namespace AZ
{
    namespace Debug
    {
        /**
         * \todo use AZStd::chrono
         */
        class Timer
        {
        public:
            /// Stores the current time in the timer.
            AZ_FORCE_INLINE void  Stamp()                                   { m_timeStamp = AZStd::GetTimeNowTicks(); }
            /// Return delta time from the last MarkTime() in seconds.
            AZ_FORCE_INLINE float GetDeltaTimeInSeconds() const             { return (float)(AZStd::GetTimeNowTicks() - m_timeStamp) /  (float)AZStd::GetTimeTicksPerSecond(); }
            /// Return delta time from the last MarkTime() in ticks.
            AZ_FORCE_INLINE AZStd::sys_time_t GetDeltaTimeInTicks() const   { return AZStd::GetTimeNowTicks() - m_timeStamp; }
            /// Return delta time from the last MarkTime() in seconds and sets sets the current time.
            AZ_FORCE_INLINE float StampAndGetDeltaTimeInSeconds()
            {
                AZStd::sys_time_t curTime = AZStd::GetTimeNowTicks();
                float timeInSec = (float)(curTime - m_timeStamp) /  (float)AZStd::GetTimeTicksPerSecond();
                m_timeStamp = curTime;
                return timeInSec;
            }
            /// Return delta time from the last MarkTime() in ticks and sets sets the current time.
            AZ_FORCE_INLINE AZStd::sys_time_t StampAndGetDeltaTimeInTicks()
            {
                AZStd::sys_time_t curTime = AZStd::GetTimeNowTicks();
                AZStd::sys_time_t timeInTicks = curTime - m_timeStamp;
                m_timeStamp = curTime;
                return timeInTicks;
            }
        private:
            AZStd::sys_time_t m_timeStamp;
        };
    }
}
