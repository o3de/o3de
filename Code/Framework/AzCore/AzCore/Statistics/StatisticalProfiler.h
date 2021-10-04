/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/BusImpl.h> //Just to get AZ::NullMutex
#include <AzCore/Statistics/StatisticsManager.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ
{
    namespace Statistics
    {
        //! A helper class that facilitates collecting time spent in blocks (scopes) of code
        //! and aggregating the measured times as running statistics.
        //!
        //! See "StatisticalProfilerProxy.h" for more explanations on the meaning of Statistical Profiling.
        //!
        //! The StatisticalProfiler was made as a template to accommodate for several performance needs...
        //! If all the code that is being profiled is single threaded and you want to identify
        //! each statistic by its string name, then the default StatisticalProfiler<> works for you.
        //! If using a map<strings, stat> is too much of what you can afford, then index your
        //! statistics with an integer or crc32 and your code profiler should be declared as
        //! StatisticalProfiler<AZ::Crc32>.
        //! For multi-threaded cases and indexing statistic with Crc32 you can have a profiler like this:
        //! StatisticalProfiler<AZ::Crc32, AZStd::mutex>.
        //! The UnitTests mentioned in the first paragraph do benchmarks of different combinations
        //! of indexing and synchronization primitives.
        //!
        //! Even though you can create, subclass and use your own StatisticalProfiler<*,*>, there
        //! are some things to consider when working with the StatisticalProfilerProxy:
        //! The StatisticalProfilerProxy OWNS an array of StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex>.
        //! You can "manage" one of those StatisticalProfiler by getting a reference to it and
        //! add Running statistics etc.
        template <class StatIdType = AZStd::string, class MutexType = AZ::NullMutex>
        class StatisticalProfiler
        {
        public:

            //! A Convenience class used to measure time performance of scopes of code
            //! with constructor/destructor. Suitable to be used as part of a macro
            //! to facilitate its usage.
            class TimedScope
            {
            public:
                TimedScope() = delete;

                TimedScope(StatisticalProfiler& profiler, const StatIdType& statId)
                    : m_profiler(profiler), m_statId(statId)
                {
                    m_startTime = AZStd::chrono::high_resolution_clock::now();
                }

                ~TimedScope()
                {
                    AZStd::chrono::system_clock::time_point stopTime = AZStd::chrono::high_resolution_clock::now();
                    AZStd::chrono::microseconds duration = stopTime - m_startTime;
                    m_profiler.PushSample(m_statId, static_cast<double>(duration.count()));
                }

            private:
                StatisticalProfiler& m_profiler;
                const StatIdType& m_statId;
                AZStd::chrono::system_clock::time_point m_startTime;
            }; //class TimedScope

            friend class TimedScope;

            StatisticalProfiler()
            {
            }

            StatisticalProfiler(const StatisticalProfiler& other)
            {
                m_statisticsManager = other.m_statisticsManager;
                m_statsVector.clear();
                m_perFrameAggregates.clear();
            }

            StatisticalProfiler(StatisticalProfiler&& other)
            {
                m_statisticsManager = AZStd::move(other.m_statisticsManager);
                m_perFrameAggregates = AZStd::move(other.m_perFrameAggregates);
            }

            virtual ~StatisticalProfiler()
            {
            }

            AZ::Statistics::StatisticsManager<StatIdType>& GetStatsManager()
            {
                return m_statisticsManager;
            }

            //! Should be called once per frame, it runs over all existing timed stats in m_statsForPerFrameCalculation
            //! and accumulates all the values as a single stat per frame.
            double SummarizePerFrameStats()
            {
                AZStd::scoped_lock<MutexType> lock(m_mutex);

                if (m_perFrameAggregates.size() < 1)
                {
                    return 0.0;
                }

                double allStatsSumMicroSecs = 0.0;

                for (StatisticalAggregate& aggregate : m_perFrameAggregates)
                {
                    double statsSumMicroSecs = 0.0;
                    for (const AZ::Statistics::NamedRunningStatistic* stat : aggregate.m_statsForPerFrameCalculation)
                    {
                        statsSumMicroSecs += stat->GetSum();
                    }

                    const double frameTime = statsSumMicroSecs - aggregate.m_prevAccumulatedSums;
                    if (frameTime > 0.0)
                    {
                        aggregate.m_statPerFrame->PushSample(frameTime);
                        aggregate.m_prevAccumulatedSums = statsSumMicroSecs;
                    }
                    allStatsSumMicroSecs += statsSumMicroSecs;
                }

                return allStatsSumMicroSecs;
            }

            void LogAndResetStats(const char* windowName)
            {
                AZStd::scoped_lock<MutexType> lock(m_mutex);

                if (m_statsVector.size() != m_statisticsManager.GetCount())
                {
                    m_statsVector.clear();
                    m_statisticsManager.GetAllStatistics(m_statsVector);
                }

                for (AZ::Statistics::NamedRunningStatistic* stat : m_statsVector)
                {
                    if (stat->GetNumSamples() == 0)
                    {
                        continue;
                    }
                    const AZStd::string& statReport = stat->GetFormatted();
                    AZ_Printf(windowName, "%s\n", statReport.c_str());
                    stat->Reset();
                }
                for (StatisticalAggregate& aggregate : m_perFrameAggregates)
                {
                    aggregate.m_prevAccumulatedSums = 0.0;
                }
            }

            void PushSample(const StatIdType& statId, double value)
            {
                AZStd::scoped_lock<MutexType> lock(m_mutex);
                AZ::Statistics::NamedRunningStatistic* stat = m_statisticsManager.GetStatistic(statId);
                if (!stat)
                {
                    return;
                }
                stat->PushSample(value);
            }

            const AZ::Statistics::NamedRunningStatistic* GetStatistic(const StatIdType& statId)
            {
                return m_statisticsManager.GetStatistic(statId);
            }

            int AddPerFrameStatisticalAggregate(const AZStd::vector<StatIdType>& statIds,
                const StatIdType& timePerFrameStatId,
                const AZStd::string& timePerFrameStatName)
            {
                AZStd::scoped_lock<MutexType> lock(m_mutex);

                m_perFrameAggregates.push_back(StatisticalAggregate());
                StatisticalAggregate& aggregate = m_perFrameAggregates[m_perFrameAggregates.size() - 1];

                int added_count = 0;
                for (const StatIdType& statId : statIds)
                {
                    AZ::Statistics::NamedRunningStatistic* stat = m_statisticsManager.GetStatistic(statId);
                    if (!stat)
                    {
                        continue;
                    }
                    auto const& itor = AZStd::find(aggregate.m_statsForPerFrameCalculation.begin(), aggregate.m_statsForPerFrameCalculation.end(), stat);
                    if (itor != aggregate.m_statsForPerFrameCalculation.end())
                    {
                        continue;
                    }
                    aggregate.m_statsForPerFrameCalculation.push_back(stat);
                    added_count++;
                }

                if (added_count < 1)
                {
                    m_perFrameAggregates.pop_back();
                    return 0;
                }

                aggregate.m_statPerFrame = m_statisticsManager.AddStatistic(timePerFrameStatId, timePerFrameStatName, "us", true);
                if (!aggregate.m_statPerFrame)
                {
                    AZ_Warning("StatisticalProfiler", false, "Per frame stat with name %s already exists\n", timePerFrameStatName.c_str());
                    m_perFrameAggregates.pop_back();
                    return 0;
                }

                return added_count;
            }

            const AZ::Statistics::NamedRunningStatistic* GetFirstStatPerFrame() const
            {
                if (m_perFrameAggregates.size() < 1)
                {
                    return nullptr;
                }
                return m_perFrameAggregates[0].m_statPerFrame;
            }

        protected:
            //! Lock this before reading/writing to m_timeStatisticsManager, or else...
            MutexType m_mutex;
            AZ::Statistics::StatisticsManager<StatIdType> m_statisticsManager;
            AZStd::vector<AZ::Statistics::NamedRunningStatistic*> m_statsVector;


            struct StatisticalAggregate
            {
                StatisticalAggregate() : m_statPerFrame(nullptr), m_prevAccumulatedSums(0.0)
                {

                }
                AZ::Statistics::NamedRunningStatistic* m_statPerFrame;
                AZStd::vector<AZ::Statistics::NamedRunningStatistic*> m_statsForPerFrameCalculation;

                //! This one is needed because running statistics are collected many times across
                //! several frames. This value is used to calculate a per frame sample for @m_totalTimePerFrameStat,
                //! by subtracting @m_prevAccumulatedSums from the accumulated sum in @m_statisticsManager.
                double m_prevAccumulatedSums;
            };

            AZStd::vector<StatisticalAggregate> m_perFrameAggregates;

        }; //class StatisticalProfiler

    }; //namespace Statistics
}; //namespace AZ
