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
    } // namespace Platform

    template<typename... T>
    void ProfileScope::BeginRegion(
        [[maybe_unused]] Budget* budget, [[maybe_unused]] bool budgetTotal, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args)
    {
    #if !defined(_RELEASE)
        if (budget)
        {
            if (!budgetTotal)
            {
                Platform::BeginProfileRegion(budget, eventName, args...);

                if (auto profiler = AZ::Interface<Profiler>::Get(); profiler)
                {
                    profiler->BeginRegion(budget, eventName, sizeof...(T), args...);
                }
            }
            else
            {
                budget->BeginProfileRegion();
            }
        }
    #endif // !defined(_RELEASE)
    }

    inline void ProfileScope::EndRegion([[maybe_unused]] Budget* budget, [[maybe_unused]] bool budgetTotal)
    {
    #if !defined(_RELEASE)
        if (budget)
        {
            if (!budgetTotal)
            {
                if (auto profiler = AZ::Interface<Profiler>::Get(); profiler)
                {
                    profiler->EndRegion(budget);
                }

                Platform::EndProfileRegion(budget);
            }
            else
            {
                budget->EndProfileRegion();
            }
        }
    #endif // !defined(_RELEASE)
    }

    template<typename... T>
    ProfileScope::ProfileScope(Budget* budget, bool budgetTotal, const char* eventName, T const&... args)
        : m_budget{ budget }
        , m_budgetTotal { budgetTotal }
    {
        BeginRegion(budget, budgetTotal, eventName, args...);
    }

    inline ProfileScope::~ProfileScope()
    {
        EndRegion(m_budget, m_budgetTotal);
    }

} // namespace AZ::Debug

#include <AzCore/Debug/Profiler_Platform.inl>
