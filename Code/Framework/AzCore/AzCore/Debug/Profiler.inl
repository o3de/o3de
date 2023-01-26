/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>

namespace AZ::Debug
{
    namespace Platform
    {
        template<typename... T>
        void BeginProfileRegion(Budget* budget, const char* eventName, T const&... args);
        void BeginProfileRegion(Budget* budget, const char* eventName);
        void EndProfileRegion(Budget* budget);
        template<typename T>
        void ReportCounter(const Budget* budget, const wchar_t* counterName, const T& value);
        void ReportProfileEvent(const Budget* budget, const char* eventName);
    } // namespace Platform

    template<typename... T>
    void ProfileScope::BeginRegion(
        [[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args)
    {
    #if !defined(_RELEASE)
        if (budget)
        {
            Platform::BeginProfileRegion(budget, eventName, args...);

            budget->BeginProfileRegion();

            // Initialize the cached pointer with the current handler or nullptr if no handlers are registered.
            // We do it here because Interface::Get will do a full mutex lock if no handlers are registered
            // causing big performance hit.
            if (!m_cachedProfiler.has_value())
            {
                m_cachedProfiler = AZ::Interface<Profiler>::Get();
            }

            if (m_cachedProfiler.value())
            {
                m_cachedProfiler.value()->BeginRegion(budget, eventName, sizeof...(T), args...);
            }
        }
    #endif // !defined(_RELEASE)
    }

    inline void ProfileScope::EndRegion([[maybe_unused]] Budget* budget)
    {
    #if !defined(_RELEASE)
        if (budget)
        {
            budget->EndProfileRegion();

            if (m_cachedProfiler.value())
            {
                m_cachedProfiler.value()->EndRegion(budget);
            }

            Platform::EndProfileRegion(budget);
        }
    #endif // !defined(_RELEASE)
    }

    template<typename... T>
    ProfileScope::ProfileScope(Budget* budget, const char* eventName, T const&... args)
        : m_budget{ budget }
    {
        BeginRegion(budget, eventName, args...);
    }

    inline ProfileScope::~ProfileScope()
    {
        EndRegion(m_budget);
    }

    template<typename T>
    inline void Profiler::ReportCounter(
        [[maybe_unused]] const Budget* budget, [[maybe_unused]] const wchar_t* counterName,
        [[maybe_unused]] const T& value)
    {
#if !defined(_RELEASE)
        Platform::ReportCounter(budget, counterName, value);
#endif // !defined(_RELEASE)
    }
    inline void Profiler::ReportProfileEvent([[maybe_unused]] const Budget* budget,
        [[maybe_unused]] const char* eventName)
    {
#if !defined(_RELEASE)
        Platform::ReportProfileEvent(budget, eventName);
#endif // !defined(_RELEASE)
    }

} // namespace AZ::Debug

#include <AzCore/Debug/Profiler_Platform.inl>
