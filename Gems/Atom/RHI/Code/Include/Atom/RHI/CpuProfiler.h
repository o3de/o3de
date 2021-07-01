/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace RHI
    {
        //! Structure that is used to cache a timed region into the thread's local storage.
        struct CachedTimeRegion
        {
            //! Structure that the profiling macro utilizes to create statically initialized instance to create string
            //! literals in static memory
            struct GroupRegionName
            {
                GroupRegionName() = delete;
                GroupRegionName(const char* const group, const char* const region);
                
                const char* const m_groupName = nullptr;
                const char* const m_regionName = nullptr;
            };

            CachedTimeRegion() = default;
            CachedTimeRegion(const GroupRegionName* groupRegionName);
            CachedTimeRegion(const GroupRegionName* groupRegionName, uint16_t stackDepth, uint64_t startTick, uint64_t endTick);

            //! Pointer to the GroupRegionName static instance.
            //! NOTE: When used in a separate shared library, the library mustn't be unloaded before
            //! the CpuProfiler is shutdown.
            const GroupRegionName* m_groupRegionName = nullptr;

            uint16_t m_stackDepth = 0u;
            AZStd::sys_time_t m_startTick = 0;
            AZStd::sys_time_t m_endTick = 0;
        };

        //! Helper class used as a RAII-style mechanism for the macros to begin and end a region.
        class TimeRegion : public CachedTimeRegion
        {
        public:
            TimeRegion() = delete;
            TimeRegion(const GroupRegionName* groupRegionName);
            ~TimeRegion();

            //! End region
            void EndRegion();
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

            //! Add a new time region
            virtual void BeginTimeRegion(TimeRegion& timeRegion) = 0;

            //! Ends a time region
            virtual void EndTimeRegion() = 0;

            //! Get the last frame's TimeRegionMap
            virtual const TimeRegionMap& GetTimeRegionMap() const = 0;

            //! Enable/Disable the CpuProfiler
            virtual void SetProfilerEnabled(bool enabled) = 0;

            virtual bool IsProfilerEnabled() const = 0 ;
        };

    } // namespace RPI
} // namespace AZ

//! Utility functions for timing a section of code and writing the timing (in cycles) to a new, named time region inside the
//! provided statistics data.

//! Supply a group and region to the time region
#define AZ_ATOM_PROFILE_TIME_GROUP_REGION(groupName, regionName) \
    static const AZ::RHI::CachedTimeRegion::GroupRegionName AZ_JOIN(groupRegionName, __LINE__)(groupName, regionName); \
    AZ::RHI::TimeRegion AZ_JOIN(timeRegion, __LINE__)(&AZ_JOIN(groupRegionName, __LINE__));

//! Supply a region to the time region; "Default" will be used for the group
#define AZ_ATOM_PROFILE_TIME_REGION(regionName) \
    AZ_ATOM_PROFILE_TIME_GROUP_REGION("Default", regionName)

//! Used to create a time region; "Default" will be used for the group, and __FUNCTION__ macro for the region
#define AZ_ATOM_PROFILE_TIME_FUNCTION() \
    AZ_ATOM_PROFILE_TIME_GROUP_REGION("Default", AZ_FUNCTION_SIGNATURE)

//! Macro that combines the AZ_TRACE_METHOD with time profiling macro
#define AZ_ATOM_PROFILE_FUNCTION(groupName, regionName) \
    AZ_TRACE_METHOD(); \
    AZ_ATOM_PROFILE_TIME_GROUP_REGION(groupName, regionName) \
