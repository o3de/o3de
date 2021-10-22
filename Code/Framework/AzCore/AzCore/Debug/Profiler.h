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

#ifdef USE_PIX
#include <AzCore/PlatformIncl.h>
#include <WinPixEventRuntime/pix3.h>
#endif

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
#define AZ_PROFILE_DATAPOINT(...)
#define AZ_PROFILE_DATAPOINT_PERCENT(...)
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

        // support for the extra macro args (e.g. format strings) will come in a later PR
        virtual void BeginRegion(const Budget* budget, const char* eventName) = 0;
        virtual void EndRegion(const Budget* budget) = 0;
    };

    class ProfileScope
    {
    public:
        template<typename... T>
        static void BeginRegion([[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args);

        static void EndRegion([[maybe_unused]] Budget* budget);

        template<typename... T>
        ProfileScope(Budget* budget, char const* eventName, T const&... args);

        ~ProfileScope();

    private:
        Budget* m_budget;
    };
} // namespace AZ::Debug

#include <AzCore/Debug/Profiler.inl>
