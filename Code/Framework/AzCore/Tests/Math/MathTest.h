/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>

#if AZ_DEBUG_BUILD
    #define AZ_MATH_TEST_START_TRACE_SUPPRESSION     AZ_TEST_START_TRACE_SUPPRESSION
    #define AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(x)   AZ_TEST_STOP_TRACE_SUPPRESSION(x)
#else
    #define AZ_MATH_TEST_START_TRACE_SUPPRESSION
    #define AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(x)
#endif

namespace UnitTest::Constants
{
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    // Larger tolerance is needed when using NEON.
    static const float SimdTolerance = 0.005f;
    // Low precision from NEON causes some estimate functions,
    // like SetLengthEstimate(), to need larger tolerance.
    static const float SimdToleranceEstimateFuncs = 0.05f;
    // Low precision from NEON InvSqrt() leads to AngleDeg tests to need
    // large tolerance since converting to degrees exacerbates the low precision.
    static const float SimdToleranceAngleDeg = 0.2f;
#else
    static const float SimdTolerance = AZ::Constants::Tolerance;
    static const float SimdToleranceEstimateFuncs = AZ::Constants::Tolerance;
    static const float SimdToleranceAngleDeg = AZ::Constants::Tolerance;
#endif
}
