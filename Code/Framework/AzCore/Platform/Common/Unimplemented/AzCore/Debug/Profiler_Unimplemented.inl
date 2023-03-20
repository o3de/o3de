/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ::Debug::Platform
{
    template<typename... T>
    void BeginProfileRegion([[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args)
    {
    }

    inline void BeginProfileRegion([[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName)
    {
    }

    inline void EndProfileRegion([[maybe_unused]] Budget* budget)
    {
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
