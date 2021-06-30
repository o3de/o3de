/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CpuProfilerImpl.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzCore/Debug/Timer.h>
#include <Atom/RHI/RHIUtils.h>

namespace AZ
{
    namespace RHI
    {
        thread_local CpuTimingLocalStorage* CpuProfilerImpl::ms_threadLocalStorage = nullptr;

        // --- CpuProfiler ---

        CpuProfiler* CpuProfiler::Get()
        {
            return Interface<CpuProfiler>::Get();
        }

        // --- TimeRegion ---

        TimeRegion::TimeRegion(const GroupRegionName* groupRegionName) :
            CachedTimeRegion(groupRegionName)
        {
            if (CpuProfiler::Get())
            {
                CpuProfiler::Get()->BeginTimeRegion(*this);
            }
        }

        TimeRegion::~TimeRegion()
        {
            EndRegion();
        }

        void TimeRegion::EndRegion()
        {
            if (CpuProfiler::Get())
            {
                CpuProfiler::Get()->EndTimeRegion();
            }
        }

        // --- CachedTimeRegion ---

        CachedTimeRegion::CachedTimeRegion(const GroupRegionName* groupRegionName)
        {
            m_groupRegionName = groupRegionName;
        }

        CachedTimeRegion::CachedTimeRegion(const GroupRegionName* groupRegionName, uint16_t stackDepth, uint64_t startTick, uint64_t endTick)
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

        // --- CpuProfilerImpl ---

        void CpuProfilerImpl::Init()
        {
            Interface<CpuProfiler>::Register(this);
            m_initialized = true;
            Device* rhiDevice = GetRHIDevice().get();
            FrameEventBus::Handler::BusConnect(rhiDevice);
        }

        void CpuProfilerImpl::Shutdown()
        {
            if (!m_initialized)
            {
                return;
            }
            // When this call is made, no more thread profiling calls can be performed anymore
            Interface<CpuProfiler>::Unregister(this);

            // Wait for the remaining threads that might still be processing its profiling calls
            AZStd::unique_lock<AZStd::shared_mutex> shutdownLock(m_shutdownMutex);

            m_enabled = false;

            // Cleanup all TLS
            m_registeredThreads.clear();
            m_timeRegionMap.clear();
            m_initialized = false;
            FrameEventBus::Handler::BusDisconnect();
        }

        void CpuProfilerImpl::BeginTimeRegion(TimeRegion& timeRegion)
        {
            // Try to lock here, the shutdownMutex will only be contested when the CpuProfiler is shutting down.
            if (m_shutdownMutex.try_lock_shared())
            {
                if (m_enabled)
                {
                    // Lazy initialization, creates an instance of the Thread local data if it's not created, and registers it
                    RegisterThreadStorage();

                    // Push it to the stack
                    ms_threadLocalStorage->RegionStackPushBack(timeRegion);
                }

                m_shutdownMutex.unlock_shared();
            }
        }

        void CpuProfilerImpl::EndTimeRegion()
        {
            // Try to lock here, the shutdownMutex will only be contested when the CpuProfiler is shutting down.
            if (m_shutdownMutex.try_lock_shared())
            {
                if (m_enabled)
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

        void CpuProfilerImpl::SetProfilerEnabled(bool enabled)
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_threadRegisterMutex);

            // Early out if the state is already the same
            if (m_enabled == enabled)
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

        void CpuProfilerImpl::OnFrameBegin()
        {
            if (!m_enabled)
            {
                return;
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
            AZStd::remove_if(m_registeredThreads.begin(), m_registeredThreads.end(), [](const RHI::Ptr<CpuTimingLocalStorage>& thread)
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

        void CpuTimingLocalStorage::RegionStackPushBack(TimeRegion& timeRegion)
        {
            // If it was (re)enabled, clear the lists first
            if (m_clearContainers)
            {
                m_clearContainers = false;

                m_cachedTimeRegionMap.clear();
                m_timeRegionStack.clear();
                m_cachedTimeRegions.clear();
            }

            timeRegion.m_stackDepth = m_stackLevel;

            AZ_Assert(m_timeRegionStack.size() < TimeRegionStackSize, "Adding too many time regions to the stack. Increase the size of TimeRegionStackSize.");
            m_timeRegionStack.push_back(&timeRegion);

            // Increment the stack
            m_stackLevel++;

            // Set the starting time at the end, to avoid recording the minor overhead
            timeRegion.m_startTick = AZStd::GetTimeNowTicks();
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
            TimeRegion* back = m_timeRegionStack.back();
            m_timeRegionStack.pop_back();

            // Set the ending time
            back->m_endTick = endRegionTime;

            // Decrement the stack
            m_stackLevel--;

            // Add an entry to the cached region
            AddCachedRegion(CachedTimeRegion(back->m_groupRegionName, back->m_stackDepth, back->m_startTick, back->m_endTick));
        }

        // Gets called when region ends and all data is set
        void CpuTimingLocalStorage::AddCachedRegion(CachedTimeRegion&& timeRegionCached)
        {
            if (m_hitSizeLimitMap[timeRegionCached.m_groupRegionName->m_regionName])
            {
                return;
            }
            // Add an entry to the cached region
            m_cachedTimeRegions.push_back(timeRegionCached);

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
                    const AZStd::string regionName = cachedTimeRegion.m_groupRegionName->m_regionName;
                    AZStd::vector<CachedTimeRegion>& regionVec = m_cachedTimeRegionMap[regionName];
                    regionVec.push_back(cachedTimeRegion);
                    if (regionVec.size() >= TimeRegionStackSize)
                    {
                        m_hitSizeLimitMap[cachedTimeRegion.m_groupRegionName->m_regionName] = true;
                    }
                }

                // Clear the cached regions
                m_cachedTimeRegions.clear();
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
    }
}
