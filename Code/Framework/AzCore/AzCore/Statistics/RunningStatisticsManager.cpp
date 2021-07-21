/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RunningStatisticsManager.h"

namespace AzFramework
{
    namespace Statistics
    {
        bool RunningStatisticsManager::ContainsStatistic(const AZStd::string& name)
        {
            auto iterator = m_statisticsNamesToIndexMap.find(name);
            return iterator != m_statisticsNamesToIndexMap.end();
        }

        bool RunningStatisticsManager::AddStatistic(const AZStd::string& name, const AZStd::string& units)
        {
            if (ContainsStatistic(name))
            {
                return false;
            }
            AddStatisticValidated(name, units);
            return true;
        }

        void RunningStatisticsManager::RemoveStatistic(const AZStd::string& name)
        {
            auto iterator = m_statisticsNamesToIndexMap.find(name);
            if (iterator == m_statisticsNamesToIndexMap.end())
            {
                return;
            }
            AZ::u32 itemIndex = iterator->second;
            m_statistics.erase(m_statistics.begin() + itemIndex);
            m_statisticsNamesToIndexMap.erase(iterator);
            //Update the indices in m_statisticsNamesToIndexMap.
            while (itemIndex < m_statistics.size())
            {
                const AZStd::string& statName = m_statistics[itemIndex].GetName();
                m_statisticsNamesToIndexMap[statName] = itemIndex;
                ++itemIndex;
            }
        }

        void RunningStatisticsManager::ResetStatistic(const AZStd::string& name)
        {
            NamedRunningStatistic* stat = GetStatistic(name);
            if (!stat)
            {
                return;
            }
            stat->Reset();
        }

        void RunningStatisticsManager::ResetAllStatistics()
        {
            for (NamedRunningStatistic& stat : m_statistics)
            {
                stat.Reset();
            }
        }

        void RunningStatisticsManager::PushSampleForStatistic(const AZStd::string& name, double value)
        {
            NamedRunningStatistic* stat = GetStatistic(name);
            if (!stat)
            {
                return;
            }
            stat->PushSample(value);
        }

        NamedRunningStatistic* RunningStatisticsManager::GetStatistic(const AZStd::string& name, AZ::u32* indexOut)
        {
            auto iterator = m_statisticsNamesToIndexMap.find(name);
            if (iterator == m_statisticsNamesToIndexMap.end())
            {
                return nullptr;
            }
            const AZ::u32 index = iterator->second;
            if (indexOut)
            {
                *indexOut = index;
            }
            return &m_statistics[index];
        }

        const AZStd::vector<NamedRunningStatistic>& RunningStatisticsManager::GetAllStatistics() const
        {
            return m_statistics;
        }

        void RunningStatisticsManager::AddStatisticValidated(const AZStd::string& name, const AZStd::string& units)
        {
            m_statistics.emplace_back(NamedRunningStatistic(name, units));
            const AZ::u32 itemIndex = static_cast<AZ::u32>(m_statistics.size() - 1);
            m_statisticsNamesToIndexMap[name] = itemIndex;
        }

    }//namespace Statistics
}//namespace AzFramework
