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
    void BeginRegion([[maybe_unused]] Budget* budget, const char* eventName, T const&... args)
    {
        // ideally this would be smaller but AZ_PROFILE_FUNCTION produces some long event names
        using EventNameString = AZStd::fixed_string<512>;

    AZ_PUSH_DISABLE_WARNING(, "-Wformat-security")
        EventNameString fullEventName = EventNameString::format(eventName, args...);
    AZ_POP_DISABLE_WARNING

        ATrace_beginSection(fullEventName.c_str());
    }

    inline void BeginRegion([[maybe_unused]] Budget* budget, const char* eventName)
    {
        ATrace_beginSection(eventName);
    }

    inline void EndRegion([[maybe_unused]] Budget* budget)
    {
        ATrace_endSection();
    }
} // namespace AZ::Debug::Platform
