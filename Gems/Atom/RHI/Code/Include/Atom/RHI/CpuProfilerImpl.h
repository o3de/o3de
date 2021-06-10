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

#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI.Reflect/Base.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/smart_ptr/intrusive_refcount.h>

namespace AZ
{
    namespace RHI
    {
        //! Thread local class to keep track of the thread's cached time regions.
        //! Each thread keeps track of its own time regions, which is communicated from the CpuProfilerImpl.
        //! The CpuProfilerImpl is able to request the cached time regions from the CpuTimingLocalStorage.
        class CpuTimingLocalStorage :
            public AZStd::intrusive_refcount<AZStd::atomic_uint>
        {
            friend class CpuProfilerImpl;

        public:
            AZ_CLASS_ALLOCATOR(CpuTimingLocalStorage, AZ::OSAllocator, 0);

            CpuTimingLocalStorage();
            ~CpuTimingLocalStorage();

        private:
            // Maximum stack size
            static constexpr uint32_t TimeRegionStackSize = 2048u;

            // Adds a region to the stack, gets called each time a region begins
            void RegionStackPushBack(TimeRegion& timeRegion);

            // Pops a region from the stack, gets called each time a region ends
            void RegionStackPopBack();

            // Add a new cached time region. If the stack is empty, flush all entries to the cached map
            void AddCachedRegion(CachedTimeRegion&& timeRegionCached);

            // Tries to flush the map to the passed parameter, only if the thread's mutex is unlocked
            void TryFlushCachedMap(CpuProfiler::ThreadTimeRegionMap& cachedRegionMap);

            AZStd::thread_id m_executingThreadId;
            // Keeps track of the current thread's stack depth
            uint32_t m_stackLevel = 0u;

            // Cached region map, will be flushed to the system's map when the system requests it
            CpuProfiler::ThreadTimeRegionMap m_cachedTimeRegionMap;

            // Use fixed vectors to avoid re-allocating new elements
            // Keeps track of the regions that added and removed using the macro
            AZStd::fixed_vector<TimeRegion*, TimeRegionStackSize> m_timeRegionStack;

            // Keeps track of regions that completed (i.e regions that was pushed and popped from the stack)
            // Intermediate storage point for the CachedTimeRegions, when the stack is empty, all entries will be
            // copied to the map.
            AZStd::fixed_vector<CachedTimeRegion, TimeRegionStackSize> m_cachedTimeRegions;
            AZStd::mutex m_cachedTimeRegionMutex;

            // Dirty flag which is set when the CpuProfiler's enabled state is set from false to true
            AZStd::atomic_bool m_clearContainers = false;

            // When the thread is terminated, it will flag itself for deletion
            AZStd::atomic_bool m_deleteFlag = false;
        };

        //! CpuProfiler will keep track of the registered threads, and
        //! forwards the request to profile a region to the appropriate thread. The user is able to request all
        //! cached regions, which are stored on a per thread frequency.
        class CpuProfilerImpl final
            : public CpuProfiler
        {
            friend class CpuTimingLocalStorage;

        public:
            AZ_TYPE_INFO(CpuProfilerImpl, "{10E9D394-FC83-4B45-B2B8-807C6BF07BF0}");
            AZ_CLASS_ALLOCATOR(CpuProfilerImpl, AZ::OSAllocator, 0);

            CpuProfilerImpl() = default;
            ~CpuProfilerImpl() = default;

            //! Registers the CpuProfilerImpl instance to the interface
            void Init();
            //! Unregisters the CpuProfilerImpl instance from the interface 
            void Shutdown();

            //! CpuProfiler overrides...
            void BeginTimeRegion(TimeRegion& timeRegion) final;
            void EndTimeRegion() final;
            void FlushTimeRegionMap(TimeRegionMap& timeRegionMap) final;
            void SetProfilerEnabled(bool enabled) final;

        private:
            // Lazily create and register the local thread data
            void RegisterThreadStorage();

            // ThreadId -> ThreadTimeRegionMap
            // When the user requests the cached time regions from the system, it will use this map as an intermediate
            // storage point to flush each thread's cached regions into this map.
            TimeRegionMap m_timeRegionMap;

            // Set of registered threads when created
            AZStd::vector<RHI::Ptr<CpuTimingLocalStorage>, AZ::OSStdAllocator> m_registeredThreads;
            AZStd::mutex m_threadRegisterMutex;

            // Thread local storage, gets lazily allocated when a thread is created
            static thread_local CpuTimingLocalStorage* ms_threadLocalStorage;

            // Enable/Disables the threads from profiling
            AZStd::atomic_bool m_enabled = false;

            // This lock will only be contested when the CpuProfiler's Shutdown() method has been called
            AZStd::shared_mutex m_shutdownMutex;

            bool m_initialized = false;
        };

    }; // namespace RPI
}; // namespace AZ
