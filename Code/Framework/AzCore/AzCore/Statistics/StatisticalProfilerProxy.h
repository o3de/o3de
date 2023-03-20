/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Statistics/StatisticalProfiler.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/shared_mutex.h>


namespace AZ::Statistics
{
    using StatisticalProfilerId = uint32_t;

    //! This AZ::Interface<> (Yes, it is an application wide singleton) owns an array of StatisticalProfilers.
    //! When is this useful?
    //! When you need to statistically profile code that runs across DLL boundaries.
    //!
    //! What is the meaning of "statistically profile" code?
    //! In regular profiling with tools like RAD Telemetry, every execution of a profiled
    //! scope of code will be captured when using AZ_PROFILE_SCOPE(). You can collect
    //! very large amounts of data and do your own post processing and analysis in tools like Excel,etc.
    //! In contrast, "statistical profiling" means that everytime AZ_PROFILE_SCOPE() is called,
    //! the time spent in the given scope of code will be mathematically accumulated as part of a unique
    //! Running statistic. Common statistical parameters like min, max, average, variance and standard deviation
    //! are calculated on the fly. This approach reduces considerably the amount of data that is collected.
    //! The data is recorded in the Game/Editor Log file.
    //!
    //! This StatisticalProfilerProxy should be used via the AZ_PROFILE_SCOPE() macro, and by using
    //! this macro the developer gains the flexibility of switching at compile time between profiling
    //! the code via RAD Telemetry or through statistical profiling.
    //!
    //! When creating a new statistical profiler add your category (aka profiler id) in Profiler.h (enum class ProfileCategory).
    //! Get a reference of the statistical profiler with "GetProfiler(const StatisticalProfilerId& id)" using the profiler Id.
    //! Once you get a reference to the profiler you can customize it, add Running statistics to it, etc.
    //! Some class in your code will manage the reference to the statistical profiler and will determine
    //! the policy on how often to log data to the game logs, etc. For example, by subclassing the TickBus Handler, etc.
    //!
    //! The StatisticalProfilerProxySystemComponent guarantees that the StatisticalProfilerProxy singleton exists
    //! as soon as the AZ::Environment is fully initialized.
    //! See StatisticalProfiler.h for more details and info.
    class StatisticalProfilerProxy
    {
    public:
        AZ_TYPE_INFO(StatisticalProfilerProxy, "{1103D0EB-1C32-4854-B9D9-40A2D65BDBD2}");

        using StatIdType = AZ::Crc32;
        using StatisticalProfilerType = StatisticalProfiler<StatIdType, AZStd::shared_mutex>;

        //! A Convenience class used to measure time performance of scopes of code
        //! with constructor/destructor. Suitable to be used as part of a macro
        //! to facilitate its usage.
        class TimedScope
        {
        public:
            TimedScope() = delete;

            TimedScope(const StatisticalProfilerId profilerId, const StatIdType& statId)
                : m_profilerId(profilerId)
                , m_statId(statId)
            {
                if (!m_profilerProxy)
                {
                    m_profilerProxy = AZ::Interface<StatisticalProfilerProxy>::Get();
                    if (!m_profilerProxy)
                    {
                        return;
                    }
                }
                if (!m_profilerProxy->IsProfilerActive(profilerId))
                {
                    return;
                }
                m_startTime = AZStd::chrono::steady_clock::now();
            }

            ~TimedScope()
            {
                if (!m_profilerProxy)
                {
                    return;
                }
                auto stopTime = AZStd::chrono::steady_clock::now();
                auto duration = stopTime - m_startTime;
                m_profilerProxy->PushSample(m_profilerId, m_statId, static_cast<double>(duration.count()));
            }

            //! Required only for UnitTests
            static void ClearCachedProxy()
            {
                m_profilerProxy = nullptr;
            }

        private:
            static StatisticalProfilerProxy* m_profilerProxy;
            const StatisticalProfilerId m_profilerId;
            const StatIdType& m_statId;
            AZStd::chrono::steady_clock::time_point m_startTime;
        }; // class TimedScope

        friend class TimedScope;

        StatisticalProfilerProxy()
        {
            AZ::Interface<StatisticalProfilerProxy>::Register(this);
        }

        virtual ~StatisticalProfilerProxy()
        {
            AZ::Interface<StatisticalProfilerProxy>::Unregister(this);
        }

        // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
        StatisticalProfilerProxy(StatisticalProfilerProxy&&) = delete;
        StatisticalProfilerProxy& operator=(StatisticalProfilerProxy&&) = delete;

        void RegisterProfilerId(StatisticalProfilerId id)
        {
            m_profilers.try_emplace(id, ProfilerInfo());
        }

        bool IsProfilerActive(StatisticalProfilerId id) const
        {
            auto iter = m_profilers.find(id);
            return (iter != m_profilers.end()) ? iter->second.m_enabled : false;
        }

        StatisticalProfilerType& GetProfiler(StatisticalProfilerId id)
        {
            auto iter = m_profilers.try_emplace(id, ProfilerInfo()).first;
            return iter->second.m_profiler;
        }

        void ActivateProfiler(StatisticalProfilerId id, bool activate, bool autoCreate = true)
        {
            if (autoCreate)
            {
                auto iter = m_profilers.try_emplace(id, ProfilerInfo()).first;
                iter->second.m_enabled = activate;
            }
            else if (auto iter = m_profilers.find(id); iter != m_profilers.end())
            {
                iter->second.m_enabled = activate;
            }
        }

        void PushSample(StatisticalProfilerId id, const StatIdType& statId, double value)
        {
            if (auto iter = m_profilers.find(id); iter != m_profilers.end())
            {
                iter->second.m_profiler.PushSample(statId, value);
            }
        }

        void GetAllStatistics(AZStd::vector<NamedRunningStatistic*>& stats)
        {
            for (auto& iter : m_profilers)
            {
                iter.second.m_profiler.GetStatsManager().GetAllStatistics(stats);
            }
        }

        void GetAllStatisticsOfUnits(AZStd::vector<NamedRunningStatistic*>& stats, const char* units)
        {
            for (auto& iter : m_profilers)
            {
                iter.second.m_profiler.GetStatsManager().GetAllStatisticsOfUnits(stats, units);
            }
        }
        
        void ResetAllStatistics()
        {
            for (auto& iter : m_profilers)
            {
                iter.second.m_profiler.GetStatsManager().ResetAllStatistics();
            }
        }

    private:
        struct ProfilerInfo
        {
            StatisticalProfilerType m_profiler;
            bool m_enabled{ false };
        };

        using ProfilerMap = AZStd::unordered_map<StatisticalProfilerId, ProfilerInfo>;

        ProfilerMap m_profilers;
    }; // class StatisticalProfilerProxy

}; // namespace AZ::Statistics
