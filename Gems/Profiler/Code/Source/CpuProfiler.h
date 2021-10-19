/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/ring_buffer.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace Profiler
{
    //! Structure that is used to cache a timed region into the thread's local storage.
    struct CachedTimeRegion
    {
        //! Structure used internally for caching assumed global string pointers (ideally literals) to the marker group/region
        //! NOTE: When used in a separate shared library, the library mustn't be unloaded before the CpuProfiler is shutdown.
        struct GroupRegionName
        {
            GroupRegionName() = delete;
            GroupRegionName(const char* const group, const char* const region);

            const char* m_groupName = nullptr;
            const char* m_regionName = nullptr;

            struct Hash
            {
                AZStd::size_t operator()(const GroupRegionName& name) const;
            };
            bool operator==(const GroupRegionName& other) const;
        };

        CachedTimeRegion() = default;
        explicit CachedTimeRegion(const GroupRegionName& groupRegionName);
        CachedTimeRegion(const GroupRegionName& groupRegionName, uint16_t stackDepth, uint64_t startTick, uint64_t endTick);

        GroupRegionName m_groupRegionName{nullptr, nullptr};

        uint16_t m_stackDepth = 0u;
        AZStd::sys_time_t m_startTick = 0;
        AZStd::sys_time_t m_endTick = 0;
    };

    //! Interface class of the CpuProfiler
    class CpuProfiler
    {
    public:
        using ThreadTimeRegionMap = AZStd::unordered_map<AZStd::string, AZStd::vector<CachedTimeRegion>>;
        using TimeRegionMap = AZStd::unordered_map<AZStd::thread_id, ThreadTimeRegionMap>;

        AZ_RTTI(CpuProfiler, "{127C1D0B-BE05-4E18-A8F6-24F3EED2ECA6}");

        CpuProfiler() = default;
        virtual ~CpuProfiler() = default;

        AZ_DISABLE_COPY_MOVE(CpuProfiler);

        static CpuProfiler* Get();

        //! Get the last frame's TimeRegionMap
        virtual const TimeRegionMap& GetTimeRegionMap() const = 0;

        //! Begin a continuous capture. Blocks the profiler from being toggled off until EndContinuousCapture is called.
        [[nodiscard]] virtual bool BeginContinuousCapture() = 0;

        //! Flush the CPU Profiler's saved data into the passed ring buffer .
        [[nodiscard]] virtual bool EndContinuousCapture(AZStd::ring_buffer<TimeRegionMap>& flushTarget) = 0;

        virtual bool IsContinuousCaptureInProgress() const = 0;

        //! Enable/Disable the CpuProfiler
        virtual void SetProfilerEnabled(bool enabled) = 0;

        virtual bool IsProfilerEnabled() const = 0 ;
    };
} // namespace Profiler
