/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Budget.h"

#include <AzCore/Module/Environment.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Statistics/StatisticalProfilerProxy.h>

AZ_DEFINE_BUDGET(Animation);
AZ_DEFINE_BUDGET(Audio);
AZ_DEFINE_BUDGET(AzCore);
AZ_DEFINE_BUDGET(Editor);
AZ_DEFINE_BUDGET(Entity);
AZ_DEFINE_BUDGET(Game);
AZ_DEFINE_BUDGET(System);
AZ_DEFINE_BUDGET(Physics);

namespace AZ::Debug
{
    struct BudgetImpl
    {
        AZ_CLASS_ALLOCATOR(BudgetImpl, AZ::SystemAllocator, 0);

        BudgetImpl(AZ::Statistics::StatisticalProfilerProxy::StatisticalProfilerType& profiler)
            : m_profiler(profiler)
        {
        }

        AZ::Statistics::StatisticalProfilerProxy::StatisticalProfilerType& m_profiler;
        AZStd::unordered_map<AZ::u64, AZ::u64> m_perThreadDuration;
        AZStd::atomic<bool> m_logging{ false };
        AZStd::mutex m_mapMutex;

        constexpr static int MaxDepth = 16;
        thread_local static AZStd::chrono::system_clock::time_point s_startTime[MaxDepth];
        thread_local static int s_currentDepth;
    };

    thread_local AZStd::chrono::system_clock::time_point BudgetImpl::s_startTime[BudgetImpl::MaxDepth];
    thread_local int BudgetImpl::s_currentDepth{ -1 };

    Budget::Budget(const char* name)
        : Budget( name, Crc32(name) )
    {
    }

    Budget::Budget(const char* name, uint32_t crc)
        : m_name{ name }
        , m_crc{ crc }
    {
        m_impl = aznew BudgetImpl(AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get()->GetProfiler(m_crc));
    }

    Budget::~Budget()
    {
        if (m_impl)
        {
            delete m_impl;
        }
    }

    void Budget::PerFrameReset()
    {
        // This is currently called at the beginning of every frame for all budgets.
        // It has been suggested to call this at more appropriate locations for different budgets.
        // When that change is made, we would need to ensure that each budget's
        // per-frame reset is called from the correct thread when a budget needs to be measured
        // across multiple threads.
        if (m_impl && m_impl->m_logging)
        {
            AZStd::scoped_lock mapLock(m_impl->m_mapMutex);
            for (auto& threadId : m_impl->m_perThreadDuration)
            {
                // For each thread for this budget, we need to push the frame total numbers to the 
                // statistical profiler.
                AZ::Crc32 threadIdCrc = AZ::Crc32(&threadId.first, sizeof(AZ::u64));
                m_impl->m_profiler.GetStatsManager().AddStatistic(threadIdCrc, m_name, "us");
                m_impl->m_profiler.PushSample(threadIdCrc, static_cast<double>(threadId.second));
            }
            m_impl->m_perThreadDuration.clear();
        }
    }

    void Budget::BeginProfileRegion()
    {
        if (m_impl && m_impl->m_logging)
        {
            // Different budgets can call this from the same thread in the same callstack.
            // So we need to maintain a stack of numbers
            BudgetImpl::s_currentDepth++;
            BudgetImpl::s_startTime[BudgetImpl::s_currentDepth] = AZStd::chrono::high_resolution_clock::now();
        }
    }

    void Budget::EndProfileRegion()
    {
        if (m_impl && m_impl->m_logging && BudgetImpl::s_currentDepth >= 0)
        {
            AZStd::chrono::microseconds duration = AZStd::chrono::high_resolution_clock::now() - BudgetImpl::s_startTime[BudgetImpl::s_currentDepth];
            BudgetImpl::s_currentDepth--;
            thread_local static AZ::u64 threadId = azlossy_caster(AZStd::hash<AZStd::thread_id>{}(AZStd::this_thread::get_id()));

            {
                // The same budget can be measured across multiple threads.
                // We want to measure each thread separately, so we store the numbers
                // in a map. Lock is needed since this function can be called from 
                // multiple threads simultaneously. If we wanted to avoid using the map/lock,
                // we may need to use the event logger and log all numbers and compute avg,
                // min, max, etc., with some post processing. Not sure if there's a simpler
                // way.
                AZStd::scoped_lock mapLock(m_impl->m_mapMutex);
                auto it = m_impl->m_perThreadDuration.try_emplace(threadId, duration.count());
                if (!it.second)
                {
                    it.first->second += duration.count();
                }
            }
        }
    }

    void Budget::TrackAllocation(uint64_t)
    {
    }

    void Budget::UntrackAllocation(uint64_t)
    {
    }

    void Budget::StartLogging()
    {
        m_impl->m_logging = true;
    }

    void Budget::StopLogging()
    {
        m_impl->m_logging = false;
    }
} // namespace AZ::Debug
