/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Budget.h>
#include <AzCore/Statistics/StatisticalProfilerProxy.h>

#if defined(AZ_PROFILER_MACRO_DISABLE) // by default we never disable the profiler registers as their overhead should be minimal, you can
                                       // still do that for your code though.
#define AZ_PROFILE_SCOPE(...)
#define AZ_PROFILE_FUNCTION(...)
#define AZ_PROFILE_BEGIN(...)
#define AZ_PROFILE_END(...)
#else

/**
 * Macro to declare a profile section for the current scope { }.
 * format is: AZ_PROFILE_SCOPE(categoryName, const char* formatStr, ...)
 */
#define AZ_PROFILE_SCOPE(budget, ...)                                                                                                      \
    ::AZ::Debug::ProfileScope AZ_JOIN(azProfileScope, __LINE__)                                                                            \
    {                                                                                                                                      \
        AZ_BUDGET_GETTER(budget)(), __VA_ARGS__                                                                                            \
    }

#define AZ_PROFILE_FUNCTION(category) AZ_PROFILE_SCOPE(category, AZ_FUNCTION_SIGNATURE)

// Prefer using the scoped macros which automatically end the event (AZ_PROFILE_SCOPE/AZ_PROFILE_FUNCTION)
#define AZ_PROFILE_BEGIN(budget, ...) ::AZ::Debug::ProfileScope::BeginRegion(AZ_BUDGET_GETTER(budget)(), __VA_ARGS__)
#define AZ_PROFILE_END(budget) ::AZ::Debug::ProfileScope::EndRegion(AZ_BUDGET_GETTER(budget)())

#endif // AZ_PROFILER_MACRO_DISABLE

#ifndef AZ_PROFILE_INTERVAL_START
#define AZ_PROFILE_INTERVAL_START(...)
#define AZ_PROFILE_INTERVAL_START_COLORED(...)
#define AZ_PROFILE_INTERVAL_END(...)
#define AZ_PROFILE_INTERVAL_SCOPED(budget, scopeNameId, ...) \
    static constexpr AZ::Crc32 AZ_JOIN(blockId, __LINE__)(scopeNameId); \
    AZ::Statistics::StatisticalProfilerProxy::TimedScope AZ_JOIN(scope, __LINE__)(AZ_CRC_CE(#budget), AZ_JOIN(blockId, __LINE__));

#endif

#ifndef AZ_PROFILE_DATAPOINT
#define AZ_PROFILE_DATAPOINT(budget, value, counterName)\
    ::AZ::Debug::Profiler::ReportCounter(AZ_BUDGET_GETTER(budget)(), counterName, value)
#define AZ_PROFILE_DATAPOINT_PERCENT(...)
#endif

#ifndef AZ_PROFILE_EVENT
#define AZ_PROFILE_EVENT(budget, eventName) \
    ::AZ::Debug::Profiler::ReportProfileEvent(AZ_BUDGET_GETTER(budget)(), eventName)
#endif


namespace AZStd
{
    struct thread_id; // forward declare. This is the same type as AZStd::thread::id
}

namespace AZ::Debug
{
    // interface for externally defined profiler systems
    class Profiler
    {
    public:
        AZ_RTTI(Profiler, "{3E5D6329-72D1-41BA-9158-68A349D1A4D5}");

        Profiler() = default;
        virtual ~Profiler() = default;

        virtual void BeginRegion(const Budget* budget, const char* eventName, ...) = 0;
        virtual void EndRegion(const Budget* budget) = 0;

        template<typename T>
        static void ReportCounter(const Budget* budget, const wchar_t* counterName, const T& value);
        static void ReportProfileEvent(const Budget* budget, const char* eventName);
    };

    class ProfileScope
    {
    public:
        static void BeginRegion(Budget* budget, const char* eventName, ...);
        static void EndRegion(Budget* budget);

        ProfileScope(Budget* budget, const char* eventName, ...);
        ~ProfileScope();

    private:
        Budget* m_budget;

        // Optimization to avoid calling Interface<Profiler>::Get
        static AZStd::optional<Profiler*> m_cachedProfiler;
    };
} // namespace AZ::Debug
