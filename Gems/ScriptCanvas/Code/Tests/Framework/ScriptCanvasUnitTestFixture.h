/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace ScriptCanvasUnitTest
{
    class ScriptCanvasUnitTestFixture
        : public UnitTest::LeakDetectionFixture
    {
    };

    #if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    MATCHER_P(IsClose, expected, "")
    {
        constexpr float tolerance = 0.001f;
        return arg.IsClose(expected, tolerance);
    }
    #endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
}
