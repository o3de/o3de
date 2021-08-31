/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ::Debug
{
    template<typename... T>
    void ProfileScope::BeginRegion(
        [[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args)
    {
        if (!budget)
        {
            return;
        }
#if !defined(_RELEASE)
        // TODO: Verification that the supplied system name corresponds to a known budget
#if defined(USE_PIX)
        PIXBeginEvent(PIX_COLOR_INDEX(budget->Crc() & 0xff), eventName, args...);
#endif
        budget->BeginProfileRegion();
// TODO: injecting instrumentation for other profilers
// NOTE: external profiler registration won't occur inline in a header necessarily in this manner, but the exact mechanism
//       will be introduced in a future PR
#endif
    }

    inline void ProfileScope::EndRegion([[maybe_unused]] Budget* budget)
    {
        if (!budget)
        {
            return;
        }
#if !defined(_RELEASE)
        budget->EndProfileRegion();
#if defined(USE_PIX)
        PIXEndEvent();
#endif
#endif
    }

    template<typename... T>
    ProfileScope::ProfileScope(Budget* budget, char const* eventName, T const&... args)
        : m_budget{ budget }
    {
        BeginRegion(budget, eventName, args...);
    }

    inline ProfileScope::~ProfileScope()
    {
        EndRegion(m_budget);
    }

} // namespace AZ::Debug
