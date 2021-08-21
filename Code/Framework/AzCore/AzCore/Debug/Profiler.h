/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Budget.h>

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
        *::AZ::Debug::Budget::Get<class AZ_BUDGET_NAME(budget)*>(), __VA_ARGS__                                                            \
    }

#define AZ_PROFILE_FUNCTION(category) AZ_PROFILE_SCOPE(category, AZ_FUNCTION_SIGNATURE)

// Prefer using the scoped macros which automatically end the event (AZ_PROFILE_SCOPE/AZ_PROFILE_FUNCTION)
#define AZ_PROFILE_BEGIN(budget, ...)                                                                                                      \
    ::AZ::Debug::ProfileScope::BeginRegion(*::AZ::Debug::Budget::Get<class AZ_BUDGET_NAME(budget)*>(), __VA_ARGS__)
#define AZ_PROFILE_END() ::AZ::Debug::ProfileScope::EndRegion()

#endif // AZ_PROFILER_MACRO_DISABLE

#ifndef AZ_PROFILE_INTERVAL_START
#define AZ_PROFILE_INTERVAL_START(...)
#define AZ_PROFILE_INTERVAL_START_COLORED(...)
#define AZ_PROFILE_INTERVAL_END(...)
#define AZ_PROFILE_INTERVAL_SCOPED(...)
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
    class ProfileScope
    {
    public:
        static uint32_t GetSystemID(const char* system);

        template<typename... T>
        static void BeginRegion(
            [[maybe_unused]] const Budget& budget, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args)
        {
#if !defined(_RELEASE)
            // TODO: Verification that the supplied system name corresponds to a known budget
#if defined(USE_PIX)
            PIXBeginEvent(PIX_COLOR_INDEX(budget.Crc() & 0xff), eventName, args...);
#endif
// TODO: injecting instrumentation for other profilers
// NOTE: external profiler registration won't occur inline in a header necessarily in this manner, but the exact mechanism
//       will be introduced in a future PR
#endif
        }

        static void EndRegion()
        {
#if !defined(_RELEASE)
#if defined(USE_PIX)
            PIXEndEvent();
#endif
#endif
        }

        template<typename... T>
        ProfileScope(const Budget& budget, char const* eventName, T const&... args)
        {
            BeginRegion(budget, eventName, args...);
        }

        ~ProfileScope()
        {
            EndRegion();
        }
    };
} // namespace AZ::Debug

#ifdef USE_PIX
// The pix3 header unfortunately brings in other Windows macros we need to undef
#undef DeleteFile
#undef LoadImage
#undef GetCurrentTime
#endif
