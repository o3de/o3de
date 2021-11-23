/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CpuProfilerImpl.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Statistics/StatisticalProfilerProxy.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace Profiler
{
    thread_local CpuTimingLocalStorage* CpuProfilerImpl::ms_threadLocalStorage = nullptr;

    // --- CpuProfiler ---

    CpuProfiler* CpuProfiler::Get()
    {
        return AZ::Interface<CpuProfiler>::Get();
    }

    // --- CachedTimeRegion ---

    CachedTimeRegion::CachedTimeRegion(const GroupRegionName& groupRegionName)
    {
        m_groupRegionName = groupRegionName;
    }

    CachedTimeRegion::CachedTimeRegion(const GroupRegionName& groupRegionName, uint16_t stackDepth, uint64_t startTick, uint64_t endTick)
    {
        m_groupRegionName = groupRegionName;
        m_stackDepth = stackDepth;
        m_startTick = startTick;
        m_endTick = endTick;
    }

    // --- GroupRegionName ---

    CachedTimeRegion::GroupRegionName::GroupRegionName(const char* const group, const char* const region) :
        m_groupName(group),
        m_regionName(region)
    {
    }

    AZStd::size_t CachedTimeRegion::GroupRegionName::Hash::operator()(const CachedTimeRegion::GroupRegionName& name) const
    {
        AZStd::size_t seed = 0;
        AZStd::hash_combine(seed, name.m_groupName);
        AZStd::hash_combine(seed, name.m_regionName);
        return seed;
    }

    bool CachedTimeRegion::GroupRegionName::operator==(const GroupRegionName& other) const
    {
        return (m_groupName == other.m_groupName) && (m_regionName == other.m_regionName);
    }


    // --- CpuProfilerImpl ---

    void CpuProfilerImpl::Init()
    {
        AZ::Interface<AZ::Debug::Profiler>::Register(this);
        AZ::Interface<CpuProfiler>::Register(this);
        m_initialized = true;
        AZ::SystemTickBus::Handler::BusConnect();
        m_continuousCaptureData.set_capacity(10);
    }

    void CpuProfilerImpl::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }
        // When this call is made, no more thread profiling calls can be performed anymore
        AZ::Interface<CpuProfiler>::Unregister(this);
        AZ::Interface<AZ::Debug::Profiler>::Unregister(this);

        // Wait for the remaining threads that might still be processing its profiling calls
        AZStd::unique_lock<AZStd::shared_mutex> shutdownLock(m_shutdownMutex);

        m_enabled = false;

        // Cleanup all TLS
        m_registeredThreads.clear();
        m_timeRegionMap.clear();
        m_initialized = false;
        m_continuousCaptureInProgress.store(false);
        m_continuousCaptureData.clear();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void CpuProfilerImpl::BeginRegion(const AZ::Debug::Budget* budget, const char* eventName)
    {
        // Try to lock here, the shutdownMutex will only be contested when the CpuProfiler is shutting down.
        if (m_shutdownMutex.try_lock_shared())
        {
            if (m_enabled)
            {
                // Lazy initialization, creates an instance of the Thread local data if it's not created, and registers it
                RegisterThreadStorage();

                // Push it to the stack
                CachedTimeRegion timeRegion({budget->Name(), eventName});
                ms_threadLocalStorage->RegionStackPushBack(timeRegion);
            }

            m_shutdownMutex.unlock_shared();
        }
    }

    void CpuProfilerImpl::EndRegion([[maybe_unused]] const AZ::Debug::Budget* budget)
    {
        // Try to lock here, the shutdownMutex will only be contested when the CpuProfiler is shutting down.
        if (m_shutdownMutex.try_lock_shared())
        {
            // guard against enabling mid-marker
            if (m_enabled && ms_threadLocalStorage != nullptr)
            {
                ms_threadLocalStorage->RegionStackPopBack();
            }

            m_shutdownMutex.unlock_shared();
        }
    }

    const CpuProfiler::TimeRegionMap& CpuProfilerImpl::GetTimeRegionMap() const
    {
        return m_timeRegionMap;
    }

    bool CpuProfilerImpl::BeginContinuousCapture()
    {
        bool expected = false;
        if (m_continuousCaptureInProgress.compare_exchange_strong(expected, true))
        {
            m_enabled = true;
            AZ_TracePrintf("Profiler", "Continuous capture started\n");
            return true;
        }

        AZ_TracePrintf("Profiler", "Attempting to start a continuous capture while one already in progress");
        return false;
    }

    bool CpuProfilerImpl::EndContinuousCapture(AZStd::ring_buffer<TimeRegionMap>& flushTarget)
    {
        if (!m_continuousCaptureInProgress.load())
        {
            AZ_TracePrintf("Profiler", "Attempting to end a continuous capture while one not in progress");
            return false;
        }

        if (m_continuousCaptureEndingMutex.try_lock())
        {
            m_enabled = false;
            flushTarget = AZStd::move(m_continuousCaptureData);
            m_continuousCaptureData.clear();
            AZ_TracePrintf("Profiler", "Continuous capture ended\n");
            m_continuousCaptureInProgress.store(false);

            m_continuousCaptureEndingMutex.unlock();
            return true;
        }

        return false;
    }

    bool CpuProfilerImpl::IsContinuousCaptureInProgress() const
    {
        return m_continuousCaptureInProgress.load();
    }

    void CpuProfilerImpl::SetProfilerEnabled(bool enabled)
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_threadRegisterMutex);

        // Early out if the state is already the same or a continuous capture is in progress
        if (m_enabled == enabled || m_continuousCaptureInProgress.load())
        {
            return;
        }

        // Set the dirty flag in all the TLS to clear the caches
        if (enabled)
        {
            // Iterate through all the threads, and set the clearing flag
            for (auto& threadLocal : m_registeredThreads)
            {
                threadLocal->m_clearContainers = true;
            }

            m_enabled = true;
        }
        else
        {
            m_enabled = false;
        }
    }

    bool CpuProfilerImpl::IsProfilerEnabled() const
    {
        return m_enabled;
    }

    void CpuProfilerImpl::OnSystemTick()
    {
        if (!m_enabled)
        {
            return;
        }

        if (m_continuousCaptureInProgress.load() && m_continuousCaptureEndingMutex.try_lock())
        {
            if (m_continuousCaptureData.full() && m_continuousCaptureData.size() != MaxFramesToSave)
            {
                const AZStd::size_t size = m_continuousCaptureData.size();
                m_continuousCaptureData.set_capacity(AZStd::min(MaxFramesToSave, size + size / 2));
            }

            m_continuousCaptureData.push_back(AZStd::move(m_timeRegionMap));
            m_timeRegionMap.clear();
            m_continuousCaptureEndingMutex.unlock();
        }

        AZStd::unique_lock<AZStd::mutex> lock(m_threadRegisterMutex);

        // Iterate through all the threads, and collect the thread's cached time regions
        TimeRegionMap newMap;
        for (auto& threadLocal : m_registeredThreads)
        {
            ThreadTimeRegionMap& threadMapEntry = newMap[threadLocal->m_executingThreadId];
            threadLocal->TryFlushCachedMap(threadMapEntry);
        }

        // Clear all TLS that flagged themselves to be deleted, meaning that the thread is already terminated
        AZStd::remove_if(m_registeredThreads.begin(), m_registeredThreads.end(), [](const AZStd::intrusive_ptr<CpuTimingLocalStorage>& thread)
        {
            return thread->m_deleteFlag.load();
        });

        // Update our saved time regions to the last frame's collected data
        m_timeRegionMap = AZStd::move(newMap);
    }

    void CpuProfilerImpl::RegisterThreadStorage()
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_threadRegisterMutex);
        if (!ms_threadLocalStorage)
        {
            ms_threadLocalStorage = aznew CpuTimingLocalStorage();
            m_registeredThreads.emplace_back(ms_threadLocalStorage);
        }
    }

    // --- CpuTimingLocalStorage ---

    CpuTimingLocalStorage::CpuTimingLocalStorage()
    {
        m_executingThreadId = AZStd::this_thread::get_id();
    }

    CpuTimingLocalStorage::~CpuTimingLocalStorage()
    {
        m_deleteFlag = true;
    }

    void CpuTimingLocalStorage::RegionStackPushBack(CachedTimeRegion& timeRegion)
    {
        // If it was (re)enabled, clear the lists first
        if (m_clearContainers)
        {
            m_clearContainers = false;

            m_stackLevel = 0;
            m_cachedTimeRegionMap.clear();
            m_timeRegionStack.clear();
            ResetCachedData();
        }

        timeRegion.m_stackDepth = aznumeric_cast<uint16_t>(m_stackLevel);

        AZ_Assert(m_timeRegionStack.size() < TimeRegionStackSize, "Adding too many time regions to the stack. Increase the size of TimeRegionStackSize.");
        m_timeRegionStack.push_back(timeRegion);

        // Increment the stack
        m_stackLevel++;

        // Set the starting time at the end, to avoid recording the minor overhead
        m_timeRegionStack.back().m_startTick = AZStd::GetTimeNowTicks();
    }

    void CpuTimingLocalStorage::RegionStackPopBack()
    {
        // Early out when the stack is empty, this might happen when the profiler was enabled while the thread encountered profiling markers
        if (m_timeRegionStack.empty())
        {
            return;
        }

        // Get the end timestamp here, to avoid the minor overhead
        const AZStd::sys_time_t endRegionTime = AZStd::GetTimeNowTicks();

        AZ_Assert(!m_timeRegionStack.empty(), "Trying to pop an element in the stack, but it's empty.");
        CachedTimeRegion back = m_timeRegionStack.back();
        m_timeRegionStack.pop_back();

        // Set the ending time
        back.m_endTick = endRegionTime;

        // Decrement the stack
        m_stackLevel--;

        // Add an entry to the cached region
        AddCachedRegion(back);
    }

    // Gets called when region ends and all data is set
    void CpuTimingLocalStorage::AddCachedRegion(const CachedTimeRegion& timeRegionCached)
    {
        if (m_hitSizeLimitMap[timeRegionCached.m_groupRegionName.m_regionName])
        {
            return;
        }
        // Add an entry to the cached region. Discard excess data in case there is too much to handle.
        if (m_cachedTimeRegions.size() < TimeRegionStackSize)
        {
            m_cachedTimeRegions.push_back(timeRegionCached);
        }
        // Warn only once per thread if the cached data limit has been reached.
        else if (!m_cachedDataLimitReached)
        {
            AZ_Warning(
                "Profiler", false,
                "Limit for profiling data has been reached by thread %i. Excess data will be discarded. Considering moving or reducing "
                "profiler markers to prevent data loss.",
                m_executingThreadId);
            m_cachedDataLimitReached = true;
        }

        // If the stack is empty, add it to the local cache map. Only gets called when the stack is empty
        // NOTE: this is where the largest overhead will be, but due to it only being called when the stack is empty
        // (i.e when the root region ended), this overhead won't affect any time regions.
        // The exception being for functions that are being profiled and create/spawn threads that are also profiled. Unfortunately, in this
        // case, the overhead of the profiled threads will be added to the main thread.
        if (m_timeRegionStack.empty())
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_cachedTimeRegionMutex);

            // Add the cached regions to the map
            for (auto& cachedTimeRegion : m_cachedTimeRegions)
            {
                const AZStd::string regionName = cachedTimeRegion.m_groupRegionName.m_regionName;
                AZStd::vector<CachedTimeRegion>& regionVec = m_cachedTimeRegionMap[regionName];
                regionVec.push_back(cachedTimeRegion);
                if (regionVec.size() >= TimeRegionStackSize)
                {
                    m_hitSizeLimitMap.insert_or_assign(AZStd::move(regionName), true);
                }
            }

            // Clear the cached regions
            ResetCachedData();
        }
    }

    void CpuTimingLocalStorage::TryFlushCachedMap(CpuProfiler::ThreadTimeRegionMap& cachedTimeRegionMap)
    {
        // Try to lock, if it's already in use (the cached regions in the array are being copied to the map)
        // it'll show up in the next iteration when the user requests it.
        if (m_cachedTimeRegionMutex.try_lock())
        {
            // Only flush cached time regions if there are entries available
            if (!m_cachedTimeRegionMap.empty())
            {
                cachedTimeRegionMap = AZStd::move(m_cachedTimeRegionMap);
                m_cachedTimeRegionMap.clear();
                m_hitSizeLimitMap.clear();
            }

            m_cachedTimeRegionMutex.unlock();
        }
    }

    void CpuTimingLocalStorage::ResetCachedData()
    {
        m_cachedTimeRegions.clear();
        m_cachedDataLimitReached = false;
    }

    // --- CpuProfilingStatisticsSerializer ---

    CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializer(const AZStd::ring_buffer<CpuProfiler::TimeRegionMap>& continuousData)
    {
        // Create serializable entries
        for (const auto& timeRegionMap : continuousData)
        {
            for (const auto& [threadId, regionMap] : timeRegionMap)
            {
                for (const auto& [regionName, regionVec] : regionMap)
                {
                    for (const auto& region : regionVec)
                    {
                        m_cpuProfilingStatisticsSerializerEntries.emplace_back(region, threadId);
                    }
                }
            }
        }
    }

    void CpuProfilingStatisticsSerializer::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CpuProfilingStatisticsSerializer>()
                ->Version(1)
                ->Field("cpuProfilingStatisticsSerializerEntries", &CpuProfilingStatisticsSerializer::m_cpuProfilingStatisticsSerializerEntries);
        }

        CpuProfilingStatisticsSerializerEntry::Reflect(context);
    }

    // --- CpuProfilingStatisticsSerializerEntry ---

    CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializerEntry::CpuProfilingStatisticsSerializerEntry(
        const CachedTimeRegion& cachedTimeRegion, AZStd::thread_id threadId)
    {
        m_groupName = cachedTimeRegion.m_groupRegionName.m_groupName;
        m_regionName = cachedTimeRegion.m_groupRegionName.m_regionName;
        m_stackDepth = cachedTimeRegion.m_stackDepth;
        m_startTick = cachedTimeRegion.m_startTick;
        m_endTick = cachedTimeRegion.m_endTick;
        m_threadId = AZStd::hash<AZStd::thread_id>{}(threadId);
    }

    void CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializerEntry::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CpuProfilingStatisticsSerializerEntry>()
                ->Version(1)
                ->Field("groupName", &CpuProfilingStatisticsSerializerEntry::m_groupName)
                ->Field("regionName", &CpuProfilingStatisticsSerializerEntry::m_regionName)
                ->Field("stackDepth", &CpuProfilingStatisticsSerializerEntry::m_stackDepth)
                ->Field("startTick", &CpuProfilingStatisticsSerializerEntry::m_startTick)
                ->Field("endTick", &CpuProfilingStatisticsSerializerEntry::m_endTick)
                ->Field("threadId", &CpuProfilingStatisticsSerializerEntry::m_threadId);
        }
    }
} // namespace Profiler
