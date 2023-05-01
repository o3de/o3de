/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/fixed_string.h>

#include <android/trace.h>

namespace AZ::Debug::Platform
{
    template<typename... T>
    void BeginProfileRegion([[maybe_unused]] Budget* budget, const char* eventName, T const&... args)
    {
        // ideally this would be smaller but AZ_PROFILE_FUNCTION produces some long event names
        using EventNameString = AZStd::fixed_string<512>;

    AZ_PUSH_DISABLE_WARNING(, "-Wformat-security")
        EventNameString fullEventName = EventNameString::format(eventName, args...);
    AZ_POP_DISABLE_WARNING

        ATrace_beginSection(fullEventName.c_str());
    }

    inline void BeginProfileRegion([[maybe_unused]] Budget* budget, const char* eventName)
    {
        ATrace_beginSection(eventName);
    }

    inline void EndProfileRegion([[maybe_unused]] Budget* budget)
    {
        ATrace_endSection();
    }

    template<typename T>
    inline void ReportCounter(
        [[maybe_unused]] const Budget* budget, [[maybe_unused]] const wchar_t* counterName, [[maybe_unused]] const T& value)
    {
    }

    inline void ReportProfileEvent([[maybe_unused]] const Budget* budget, [[maybe_unused]] const char* eventName)
    {
    }
} // namespace AZ::Debug::Platform
