/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/chrono/types.h>
#include <AzCore/std/parallel/shared_spin_mutex.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/containers/bitset.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Statistics/StatisticalProfiler.h>
#include <AzCore/Debug/Profiler.h>


#if !defined(AZ_PROFILE_TELEMETRY) && defined(AZ_STATISTICAL_PROFILING_ENABLED)

#if defined(AZ_PROFILE_SCOPE)
#undef AZ_PROFILE_SCOPE
#endif // #if defined(AZ_PROFILE_SCOPE)

#define AZ_PROFILE_SCOPE(profiler, scopeNameId) \
    static_assert(profiler < AZ::Debug::ProfileCategory::Count, "Invalid profiler category"); \
    static const AZStd::string AZ_JOIN(blockName, __LINE__)(scopeNameId); \
    AZ::Statistics::StatisticalProfilerProxy::TimedScope AZ_JOIN(scope, __LINE__)(profiler, AZ_JOIN(blockName, __LINE__));

#endif //#if !defined(AZ_PROFILE_TELEMETRY)

namespace AZ
{
    namespace Statistics
    {
        using StatisticalProfilerId = AZ::Debug::ProfileCategory;

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

            using StatIdType = AZStd::string;
            using StatisticalProfilerType = StatisticalProfiler<StatIdType, AZStd::shared_spin_mutex>;

            //! A Convenience class used to measure time performance of scopes of code
            //! with constructor/destructor. Suitable to be used as part of a macro
            //! to facilitate its usage.
            class TimedScope
            {
            public:
                TimedScope() = delete;

                TimedScope(const StatisticalProfilerId profilerId, const StatIdType& statId)
                    : m_profilerId(profilerId), m_statId(statId)
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
                    m_startTime = AZStd::chrono::high_resolution_clock::now();
                }
                ~TimedScope()
                {
                    if (!m_profilerProxy)
                    {
                        return;
                    }
                    AZStd::chrono::system_clock::time_point stopTime = AZStd::chrono::high_resolution_clock::now();
                    AZStd::chrono::microseconds duration = stopTime - m_startTime;
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
                AZStd::chrono::system_clock::time_point m_startTime;
            }; //class TimedScope

            friend class TimedScope;

            StatisticalProfilerProxy()
            {
                m_profilers.reserve(static_cast<AZStd::size_t>(AZ::Debug::ProfileCategory::Count));
                for (AZStd::size_t i = 0; i < static_cast<AZStd::size_t>(AZ::Debug::ProfileCategory::Count); i++)
                {
                    m_profilers.emplace_back(StatisticalProfilerType());
                }
                AZ::Interface<StatisticalProfilerProxy>::Register(this);
            }

            virtual ~StatisticalProfilerProxy()
            {
                AZ::Interface<StatisticalProfilerProxy>::Unregister(this);
            }

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            StatisticalProfilerProxy(StatisticalProfilerProxy&&) = delete;
            StatisticalProfilerProxy& operator=(StatisticalProfilerProxy&&) = delete;

            bool IsProfilerActive(StatisticalProfilerId id) const
            {
                return m_activeProfilersFlag[static_cast<AZStd::size_t>(id)];
            }

            StatisticalProfilerType& GetProfiler(StatisticalProfilerId id)
            {
                return m_profilers[static_cast<AZStd::size_t>(id)];
            }

            void ActivateProfiler(StatisticalProfilerId id, bool activate)
            {
                m_activeProfilersFlag[static_cast<AZStd::size_t>(id)] = activate;
            }

            void PushSample(StatisticalProfilerId id, const StatIdType& statId, double value)
            {
                m_profilers[static_cast<AZStd::size_t>(id)].PushSample(statId, value);
            }

        private:
            AZStd::bitset<static_cast<AZStd::size_t>(AZ::Debug::ProfileCategory::Count)> m_activeProfilersFlag;
            AZStd::vector<StatisticalProfilerType> m_profilers;
        }; //class StatisticalProfilerProxy

    }; //namespace Statistics
}; //namespace AZ
