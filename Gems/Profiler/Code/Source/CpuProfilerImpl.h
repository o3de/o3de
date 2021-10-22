/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <CpuProfiler.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/smart_ptr/intrusive_refcount.h>

namespace Profiler
{
    //! Thread local class to keep track of the thread's cached time regions.
    //! Each thread keeps track of its own time regions, which is communicated from the CpuProfilerImpl.
    //! The CpuProfilerImpl is able to request the cached time regions from the CpuTimingLocalStorage.
    class CpuTimingLocalStorage
        : public AZStd::intrusive_refcount<AZStd::atomic_uint>
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
        void RegionStackPushBack(CachedTimeRegion& timeRegion);

        // Pops a region from the stack, gets called each time a region ends
        void RegionStackPopBack();

        // Add a new cached time region. If the stack is empty, flush all entries to the cached map
        void AddCachedRegion(const CachedTimeRegion& timeRegionCached);

        // Tries to flush the map to the passed parameter, only if the thread's mutex is unlocked
        void TryFlushCachedMap(CpuProfiler::ThreadTimeRegionMap& cachedRegionMap);

        AZStd::thread_id m_executingThreadId;
        // Keeps track of the current thread's stack depth
        uint32_t m_stackLevel = 0u;

        // Cached region map, will be flushed to the system's map when the system requests it
        CpuProfiler::ThreadTimeRegionMap m_cachedTimeRegionMap;

        // Use fixed vectors to avoid re-allocating new elements
        // Keeps track of the regions that added and removed using the macro
        AZStd::fixed_vector<CachedTimeRegion, TimeRegionStackSize> m_timeRegionStack;

        // Keeps track of regions that completed (i.e regions that was pushed and popped from the stack)
        // Intermediate storage point for the CachedTimeRegions, when the stack is empty, all entries will be
        // copied to the map.
        AZStd::fixed_vector<CachedTimeRegion, TimeRegionStackSize> m_cachedTimeRegions;
        AZStd::mutex m_cachedTimeRegionMutex;

        // Dirty flag which is set when the CpuProfiler's enabled state is set from false to true
        AZStd::atomic_bool m_clearContainers = false;

        // When the thread is terminated, it will flag itself for deletion
        AZStd::atomic_bool m_deleteFlag = false;

        // Keep track of the regions that have hit the size limit so we don't have to lock to check
        AZStd::map<AZStd::string, bool> m_hitSizeLimitMap;
    };

    //! CpuProfiler will keep track of the registered threads, and
    //! forwards the request to profile a region to the appropriate thread. The user is able to request all
    //! cached regions, which are stored on a per thread frequency.
    class CpuProfilerImpl final
        : public AZ::Debug::Profiler
        , public CpuProfiler
        , public AZ::SystemTickBus::Handler
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

        // AZ::SystemTickBus::Handler overrides
        // When fired, the profiler collects all profiling data from registered threads and updates
        // m_timeRegionMap so that the next frame has up-to-date profiling data.
        void OnSystemTick() final override;

        //! AZ::Debug::Profiler overrides...
        void BeginRegion(const AZ::Debug::Budget* budget, const char* eventName)  final override;
        void EndRegion(const AZ::Debug::Budget* budget) final override;

        //! CpuProfiler overrides...
        const TimeRegionMap& GetTimeRegionMap() const final override;
        bool BeginContinuousCapture() final override;
        bool EndContinuousCapture(AZStd::ring_buffer<TimeRegionMap>& flushTarget) final override;
        bool IsContinuousCaptureInProgress() const final override;
        void SetProfilerEnabled(bool enabled) final override;
        bool IsProfilerEnabled() const final override;

    private:
        static constexpr AZStd::size_t MaxFramesToSave = 2 * 60 * 120; // 2 minutes of 120fps
        static constexpr AZStd::size_t MaxRegionStringPoolSize = 16384; // Max amount of unique strings to save in the pool before throwing warnings.

        // Lazily create and register the local thread data
        void RegisterThreadStorage();

        // ThreadId -> ThreadTimeRegionMap
        // On the start of each frame, this map will be updated with the last frame's profiling data.
        TimeRegionMap m_timeRegionMap;

        // Set of registered threads when created
        AZStd::vector<AZStd::intrusive_ptr<CpuTimingLocalStorage>, AZ::OSStdAllocator> m_registeredThreads;
        AZStd::mutex m_threadRegisterMutex;

        // Thread local storage, gets lazily allocated when a thread is created
        static thread_local CpuTimingLocalStorage* ms_threadLocalStorage;

        // Enable/Disables the threads from profiling
        AZStd::atomic_bool m_enabled = false;

        // This lock will only be contested when the CpuProfiler's Shutdown() method has been called
        AZStd::shared_mutex m_shutdownMutex;

        bool m_initialized = false;

        AZStd::mutex m_continuousCaptureEndingMutex;

        AZStd::atomic_bool m_continuousCaptureInProgress;

        // Stores multiple frames of profiling data, size is controlled by MaxFramesToSave. Flushed when EndContinuousCapture is called.
        // Ring buffer so that we can have fast append of new data + removal of old profiling data with good cache locality.
        AZStd::ring_buffer<TimeRegionMap> m_continuousCaptureData;
    };

    // Intermediate class to serialize Cpu TimedRegion data.
    class CpuProfilingStatisticsSerializer
    {
    public:
        class CpuProfilingStatisticsSerializerEntry
        {
        public:
            AZ_TYPE_INFO(CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializerEntry, "{26B78F65-EB96-46E2-BE7E-A1233880B225}");
            static void Reflect(AZ::ReflectContext* context);

            CpuProfilingStatisticsSerializerEntry() = default;
            CpuProfilingStatisticsSerializerEntry(const CachedTimeRegion& cachedTimeRegion, AZStd::thread_id threadId);

            AZ::Name m_groupName;
            AZ::Name m_regionName;
            uint16_t m_stackDepth;
            AZStd::sys_time_t m_startTick;
            AZStd::sys_time_t m_endTick;
            size_t m_threadId;
        };

        AZ_TYPE_INFO(CpuProfilingStatisticsSerializer, "{D5B02946-0D27-474F-9A44-364C2706DD41}");
        static void Reflect(AZ::ReflectContext* context);

        CpuProfilingStatisticsSerializer() = default;
        CpuProfilingStatisticsSerializer(const AZStd::ring_buffer<CpuProfiler::TimeRegionMap>& continuousData);

        AZStd::vector<CpuProfilingStatisticsSerializerEntry> m_cpuProfilingStatisticsSerializerEntries;
    };
}; // namespace Profiler
