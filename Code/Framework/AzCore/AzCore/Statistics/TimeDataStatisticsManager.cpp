/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TimeDataStatisticsManager.h"

namespace AZ
{
    namespace Statistics
    {
        void TimeDataStatisticsManager::PushTimeDataSample(const char * registerName, const AZ::Debug::ProfilerRegister::TimeData& timeData)
        {
            const AZStd::string statName(registerName);
            NamedRunningStatistic* statistic = GetStatistic(statName);
            if (!statistic)
            {
                const AZStd::string units("us");
                AddStatistic(statName, statName, units, false);
                AZ::Debug::ProfilerRegister::TimeData zeroTimeData;
                memset(&zeroTimeData, 0, sizeof(AZ::Debug::ProfilerRegister::TimeData));
                m_previousTimeData[statName] = zeroTimeData;
                statistic = GetStatistic(statName);
                AZ_Assert(statistic != nullptr, "Fatal error adding a new statistic object");
            }

            const AZ::u64 accumulatedTime = timeData.m_time;
            const AZ::s64 totalNumCalls = timeData.m_calls;
            const AZ::u64 previousAccumulatedTime = m_previousTimeData[statName].m_time;
            const AZ::s64 previousTotalNumCalls = m_previousTimeData[statName].m_calls;
            const AZ::u64 deltaTime = accumulatedTime - previousAccumulatedTime;
            const AZ::s64 deltaCalls = totalNumCalls - previousTotalNumCalls;
            
            if (deltaCalls == 0)
            {
                //This is the same old data. Let's skip it
                return;
            }

            double newSample = static_cast<double>(deltaTime) / deltaCalls;

            statistic->PushSample(newSample);
            m_previousTimeData[statName] = timeData;
        }
    } //namespace Statistics
} //namespace AZ
