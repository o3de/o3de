/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Hemisphere.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

namespace UnitTest
{
    TEST(MATH_Hemisphere, TestConstruct)
    {
        AZ::Vector3 pos = AZ::Vector3(10.0f, 10.0f, 10.0f);
        float radius = 15.0f;
        AZ::Vector3 direction = AZ::Vector3(1.0f, 0.0f, 0.0f);

        AZ::Hemisphere hemisphere(pos, radius, direction);
        EXPECT_EQ(hemisphere.GetCenter(), pos);
        EXPECT_EQ(hemisphere.GetRadius(), radius);
        EXPECT_EQ(hemisphere.GetDirection(), direction);
    }

    TEST(MATH_Hemisphere, TestSet)
    {
        AZ::Vector3 pos = AZ::Vector3(10.0f, 10.0f, 10.0f);
        float radius = 15.0f;
        AZ::Vector3 direction = AZ::Vector3(1.0f, 0.0f, 0.0f);

        AZ::Hemisphere hemisphere;
        hemisphere.SetCenter(pos);
        hemisphere.SetRadius(radius);
        hemisphere.SetDirection(direction);

        EXPECT_EQ(hemisphere.GetCenter(), pos);
        EXPECT_EQ(hemisphere.GetRadius(), radius);
        EXPECT_EQ(hemisphere.GetDirection(), direction);
    }

    TEST(MATH_Hemisphere, TestAssignment)
    {
        AZ::Vector3 pos = AZ::Vector3(10.0f, 10.0f, 10.0f);
        float radius = 15.0f;
        AZ::Vector3 direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        AZ::Hemisphere hemisphere1(pos, radius, direction);

        AZ::Vector3 newPos = AZ::Vector3(20.0f, 20.0f, 20.0f);
        float newRadius = 25.0f;
        AZ::Vector3 newDirection = AZ::Vector3(0.0f, 1.0f, 0.0f);
        AZ::Hemisphere hemisphere2(newPos, newRadius, newDirection);

        hemisphere1 = hemisphere2;
        EXPECT_EQ(hemisphere1, hemisphere2);
    }
} // namespace UnitTest
