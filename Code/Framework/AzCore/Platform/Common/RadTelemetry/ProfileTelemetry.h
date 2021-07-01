/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#ifdef AZ_PROFILE_TELEMETRY

/*!
* ProfileTelemetry.h provides a RAD Telemetry specific implementation of the AZ_PROFILE_FUNCTION, AZ_PROFILE_SCOPE, and AZ_PROFILE_SCOPE_DYNAMIC performance instrumentation markers
*/

#define TM_API_PTR g_radTmApi
#include <rad_tm.h>
#include <AzCore/Math/Color.h>

namespace ProfileTelemetryInternal
{
    inline constexpr tm_uint32 ConvertColor(uint32_t rgba)
    {
        return
            ((rgba >> 24) & 0x000000ff) | // move byte 3 to byte 0
            ((rgba << 8) & 0x00ff0000) | // move byte 1 to byte 2
            ((rgba >> 8) & 0x0000ff00) | // move byte 2 to byte 1
            ((rgba << 24) & 0xff000000);  // byte 0 to byte 3
    }

    inline constexpr tm_uint32 ConvertColor(const AZ::Color& color)
    {
        return ConvertColor(color.ToU32());
    }
}

#define  AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category) (static_cast<AZ::Debug::ProfileCategoryPrimitiveType>(1) << static_cast<AZ::Debug::ProfileCategoryPrimitiveType>(category))
// Helpers
#define AZ_INTERNAL_PROF_VERIFY_CAT(category) static_assert(category < AZ::Debug::ProfileCategory::Count, "Invalid profile category")

#define AZ_INTERNAL_PROF_MEMORY_CAT_TO_FLAGS(category) (AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category) | \
        AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(AZ::Debug::ProfileCategory::MemoryReserved))

#define AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id) static_assert(sizeof(id) <= sizeof(tm_uint64), "Interval id must be a unique value no larger than 64-bits")

#define AZ_INTERNAL_PROF_TM_FUNC_VERIFY_CAT(category, flags) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmFunction(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), flags)

#define AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, flags, ...) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmZone(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), flags, __VA_ARGS__)    

// AZ_PROFILE_FUNCTION
#define AZ_PROFILE_FUNCTION(category) \
    AZ_INTERNAL_PROF_TM_FUNC_VERIFY_CAT(category, TMZF_NONE)

#define AZ_PROFILE_FUNCTION_STALL(category) \
    AZ_INTERNAL_PROF_TM_FUNC_VERIFY_CAT(category, TMZF_STALL)

#define AZ_PROFILE_FUNCTION_IDLE(category) \
    AZ_INTERNAL_PROF_TM_FUNC_VERIFY_CAT(category, TMZF_IDLE)


// AZ_PROFILE_SCOPE
#define AZ_PROFILE_SCOPE(category, name) \
    AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_NONE, name)

#define AZ_PROFILE_SCOPE_STALL(category, name) \
    AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_STALL, name)

#define AZ_PROFILE_SCOPE_IDLE(category, name) \
     AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_IDLE, name)

// AZ_PROFILE_SCOPE_DYNAMIC
// For profiling events with dynamic scope names
// Note: the first variable argument must be a const format string
// Usage: AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory, <printf style const format string>, format args...)
#define AZ_PROFILE_SCOPE_DYNAMIC(category, ...) \
        AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_NONE, __VA_ARGS__)

#define AZ_PROFILE_SCOPE_STALL_DYNAMIC(category, ...) \
        AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_STALL, __VA_ARGS__)

#define AZ_PROFILE_SCOPE_IDLE_DYNAMIC(category, ...) \
        AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_IDLE, __VA_ARGS__)


// AZ_PROFILE_EVENT_BEGIN/END
// For profiling events that do not start and stop in the same scope (they MUST start/stop on the same thread)
// ALWAYS favor using scoped events (AZ_PROFILE_FUNCTION, AZ_PROFILE_SCOPE) as debugging an unmatched begin/end can be challenging
#define AZ_PROFILE_EVENT_BEGIN(category, name) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmEnter(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), TMZF_NONE, name)

#define AZ_PROFILE_EVENT_END(category) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmLeave(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category))


// AZ_PROFILE_INTERVAL (mapped to Telemetry Timespan APIs)
// Note: using C-style casting as we allow either pointers or integral types as IDs
#define AZ_PROFILE_INTERVAL_START(category, id, ...) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id); \
    tmBeginTimeSpan(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), (tm_uint64)(id), TMZF_NONE, __VA_ARGS__)

#define AZ_PROFILE_INTERVAL_START_COLORED(category, id, color, ...) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id); \
    tmBeginColoredTimeSpan(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), (tm_uint64)(id), 0, ProfileTelemetryInternal::ConvertColor(color), TMZF_NONE, __VA_ARGS__)

#define AZ_PROFILE_INTERVAL_END(category, id) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id); \
    tmEndTimeSpan(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), (tm_uint64)(id))

// AZ_PROFILE_INTERVAL_SCOPED
// Scoped interval event that implicitly starts and ends in the same scope
// Note: using C-style casting as we allow either pointers or integral types as IDs
// Note: the first variable argument must be a const format string
// Usage: AZ_PROFILE_INTERVAL_SCOPED(AZ::Debug::ProfileCategory, <unique interval id>, <printf style const format string>, format args...)
#define AZ_PROFILE_INTERVAL_SCOPED(category, id,  ...) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id); \
    tmTimeSpan(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), (tm_uint64)(id), TM_MIN_TIME_SPAN_TRACK_ID + static_cast<AZ::Debug::ProfileCategoryPrimitiveType>(category), 0, TMZF_NONE, __VA_ARGS__)


// AZ_PROFILE_DATAPOINT (mapped to tmPlot APIs)
// Note: data points can have static or dynamic names, if using a dynamic name the first variable argument must be a const format string
// Usage: AZ_PROFILE_DATAPOINT(AZ::Debug::ProfileCategory, <printf style const format string>, format args...)
#define AZ_PROFILE_DATAPOINT(category, value, ...) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmPlot(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), TM_PLOT_UNITS_REAL, TM_PLOT_DRAW_LINE, static_cast<double>(value), __VA_ARGS__)

#define AZ_PROFILE_DATAPOINT_PERCENT(category, value, ...) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmPlot(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), TM_PLOT_UNITS_PERCENTAGE_DIRECT, TM_PLOT_DRAW_LINE, static_cast<double>(value), __VA_ARGS__)


// AZ_PROFILE_MEMORY_ALLOC
#define AZ_PROFILE_MEMORY_ALLOC(category, address, size, context) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmAlloc(AZ_INTERNAL_PROF_MEMORY_CAT_TO_FLAGS(category), address, size, context)

#define AZ_PROFILE_MEMORY_ALLOC_EX(category, filename, lineNumber, address, size, context) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmAllocEx(AZ_INTERNAL_PROF_MEMORY_CAT_TO_FLAGS(category), filename, lineNumber, address, size, context)

#define AZ_PROFILE_MEMORY_FREE(category, address) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmFree(AZ_INTERNAL_PROF_MEMORY_CAT_TO_FLAGS(category), address)

#define AZ_PROFILE_MEMORY_FREE_EX(category, filename, lineNumber, address) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmFreeEx(AZ_INTERNAL_PROF_MEMORY_CAT_TO_FLAGS(category), filename, lineNumber, address)

#endif
