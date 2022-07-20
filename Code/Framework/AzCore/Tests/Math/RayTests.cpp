/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Math/LineSegment.h>
#include <AzCore/Math/Ray.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    TEST(MATH_Ray, Ray_CreateFromOriginAndDirection)
    {
        AZ::Ray ray(AZ::Vector3(0.0f, 0.5f, 12.0f), AZ::Vector3(3.0f, 0.0f, 4.0f).GetNormalized());
        EXPECT_THAT(ray.GetOrigin(), IsClose(AZ::Vector3(0.0f, 0.5f, 12.0f)));
        EXPECT_THAT(ray.GetDirection(), IsClose(AZ::Vector3(3.0f, 0.0f, 4.0f).GetNormalized()));
    }

    TEST(MATH_Ray, Ray_CreateFromLineSegment)
    {
        AZ::LineSegment segment(AZ::Vector3(0.0f, 0.5f, 12.0f), AZ::Vector3(15.0f, 0.0f, 3.0f));
        AZ::Ray ray = AZ::Ray::CreateFromLineSegment(segment);

        EXPECT_THAT(ray.GetOrigin(), IsClose(AZ::Vector3(0.0f, 0.5f, 12.0f)));
        EXPECT_THAT(ray.GetDirection(), IsClose((AZ::Vector3(15.0f, 0.0f, 3.0f) - AZ::Vector3(0.0f, 0.5f, 12.0f)).GetNormalized()));
    }

    TEST(MATH_Ray, Ray_CreateDirectionNonNormalizedFail)
    {
        AZ_PUSH_DISABLE_WARNING_GCC("-Wunused-but-set-variable")
        AZ_TEST_START_TRACE_SUPPRESSION;
        AZ::Ray rayUnused = AZ::Ray(AZ::Vector3(), AZ::Vector3(3, 0, 0));
#ifdef AZ_DEBUG_BUILD
        // AZ_MATH_ASSERT only asserts during debug builds, so expect an assert here
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
#else
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
#endif
        AZ_POP_DISABLE_WARNING_GCC
    }

} // namespace UnitTest
