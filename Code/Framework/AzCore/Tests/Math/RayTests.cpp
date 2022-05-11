/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Ray.h>
#include <AzCore/Math/LineSegment.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

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
}
