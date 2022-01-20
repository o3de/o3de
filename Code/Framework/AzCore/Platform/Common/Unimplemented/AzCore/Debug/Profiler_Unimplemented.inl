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
    void BeginRegion([[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args)
    {
    }

    inline void BeginRegion([[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName)
    {
    }

    inline void EndRegion([[maybe_unused]] Budget* budget)
    {
    }
} // namespace AZ::Debug::Platform
