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
