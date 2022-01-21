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
    template<typename... T>
    void ProfileScope::BeginRegion(
        [[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args)
    {
    #if !defined(_RELEASE)
        if (budget)
        {
        #if defined(USE_PIX)
            PIXBeginEvent(PIX_COLOR_INDEX(budget->Crc() & 0xff), eventName, args...);
        #endif // defined(USE_PIX)

            budget->BeginProfileRegion();

            if (auto profiler = AZ::Interface<Profiler>::Get(); profiler)
            {
                profiler->BeginRegion(budget, eventName, sizeof...(T), args...);
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

            if (auto profiler = AZ::Interface<Profiler>::Get(); profiler)
            {
                profiler->EndRegion(budget);
            }

        #if defined(USE_PIX)
            PIXEndEvent();
        #endif // defined(USE_PIX)
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

} // namespace AZ::Debug
