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

    TEST(MATH_LineSegment, LineSegment_CreateFromStartAndEnd)
    {
        AZ::LineSegment segment(AZ::Vector3(0.0f, 15.5f, 12.0f), AZ::Vector3(12.0f, 0.0f, 4.0f));
        EXPECT_THAT(segment.GetStart(), IsClose(AZ::Vector3(0.0f, 15.5f, 12.0f)));
        EXPECT_THAT(segment.GetEnd(), IsClose(AZ::Vector3(12.0f, 0.0f, 4.0f)));
    }

    TEST(MATH_LineSegment, LineSegment_CreateFromRayAndLength)
    {
        AZ::Ray ray(AZ::Vector3(0.0f, 0.5f, 12.0f), AZ::Vector3(15.0f, 0.0f, 3.0f).GetNormalized());
        AZ::LineSegment segment = AZ::LineSegment::CreateFromRayAndLength(ray, 25.0f);

        EXPECT_THAT(segment.GetStart(), IsClose(AZ::Vector3(0.0f, 0.5f, 12.0f)));
        EXPECT_THAT(segment.GetEnd(), IsClose(AZ::Vector3(0.0f, 0.5f, 12.0f) + (AZ::Vector3(15.0f, 0.0f, 3.0f).GetNormalized() * 25.0f)));
    }

    TEST(MATH_LineSegment, LineSegment_GetDifference)
    {
        AZ::LineSegment segment(AZ::Vector3(13.0f, 15.5f, 12.0f), AZ::Vector3(12.0f, 51.0f, 4.0f));

        EXPECT_THAT(segment.GetDifference(), IsClose(AZ::Vector3(-1.0f, 35.5f, -8.0f)));
    }

    TEST(MATH_LineSegment, LineSegment_GetPoint)
    {
        AZ::LineSegment segment(AZ::Vector3(4.0f, 5.0f, 6.0f), AZ::Vector3(5.0f, 10.0f, 2.0f));
        
        EXPECT_THAT(segment.GetPoint(0.5f), IsClose(AZ::Vector3(4.5f, 7.5f, 4.0f)));
    }
}
