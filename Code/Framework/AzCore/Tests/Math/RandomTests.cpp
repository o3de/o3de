/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Random.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Random, GetHaltonNumber)
    {
        EXPECT_FLOAT_EQ(0.5, GetHaltonNumber(1, 2));
        EXPECT_FLOAT_EQ(898.0f / 2187.0f, GetHaltonNumber(1234, 3));
        EXPECT_FLOAT_EQ(5981.0f / 15625.0f, GetHaltonNumber(4321, 5));
    }

    TEST(MATH_Random, HaltonSequenceStandard)
    {
        HaltonSequence<3> sequence({ 2, 3, 5 });
        auto regularSequence = sequence.GetHaltonSequence<5>();

        EXPECT_FLOAT_EQ(1.0f / 2.0f, regularSequence[0][0]);
        EXPECT_FLOAT_EQ(1.0f / 3.0f, regularSequence[0][1]);
        EXPECT_FLOAT_EQ(1.0f / 5.0f, regularSequence[0][2]);

        EXPECT_FLOAT_EQ(1.0f / 4.0f, regularSequence[1][0]);
        EXPECT_FLOAT_EQ(2.0f / 3.0f, regularSequence[1][1]);
        EXPECT_FLOAT_EQ(2.0f / 5.0f, regularSequence[1][2]);

        EXPECT_FLOAT_EQ(3.0f / 4.0f, regularSequence[2][0]);
        EXPECT_FLOAT_EQ(1.0f / 9.0f, regularSequence[2][1]);
        EXPECT_FLOAT_EQ(3.0f / 5.0f, regularSequence[2][2]);

        EXPECT_FLOAT_EQ(1.0f / 8.0f, regularSequence[3][0]);
        EXPECT_FLOAT_EQ(4.0f / 9.0f, regularSequence[3][1]);
        EXPECT_FLOAT_EQ(4.0f / 5.0f, regularSequence[3][2]);

        EXPECT_FLOAT_EQ(5.0f / 8.0f, regularSequence[4][0]);
        EXPECT_FLOAT_EQ(7.0f / 9.0f, regularSequence[4][1]);
        EXPECT_FLOAT_EQ(1.0f / 25.0f, regularSequence[4][2]);
    }
    
    TEST(MATH_Random, HaltonSequenceOffsets)
    {
        HaltonSequence<3> sequence({ 2, 3, 5 });
        sequence.SetOffsets({ 1, 2, 3 });
        auto offsetSequence = sequence.GetHaltonSequence<2>();

        EXPECT_FLOAT_EQ(1.0f / 4.0f, offsetSequence[0][0]);
        EXPECT_FLOAT_EQ(1.0f / 9.0f, offsetSequence[0][1]);
        EXPECT_FLOAT_EQ(4.0f / 5.0f, offsetSequence[0][2]);

        EXPECT_FLOAT_EQ(3.0f / 4.0f, offsetSequence[1][0]);
        EXPECT_FLOAT_EQ(4.0f / 9.0f, offsetSequence[1][1]);
        EXPECT_FLOAT_EQ(1.0f / 25.0f, offsetSequence[1][2]);
    }
    
    TEST(MATH_Random, HaltonSequenceIncrements)
    {
        HaltonSequence<3> sequence({ 2, 3, 5 });
        sequence.SetOffsets({ 1, 2, 3 });
        sequence.SetIncrements({ 1, 2, 3 });
        auto incrementedSequence = sequence.GetHaltonSequence<2>();

        EXPECT_FLOAT_EQ(1.0f / 4.0f, incrementedSequence[0][0]);
        EXPECT_FLOAT_EQ(1.0f / 9.0f, incrementedSequence[0][1]);
        EXPECT_FLOAT_EQ(4.0f / 5.0f, incrementedSequence[0][2]);

        EXPECT_FLOAT_EQ(3.0f / 4.0f, incrementedSequence[1][0]);
        EXPECT_FLOAT_EQ(7.0f / 9.0f, incrementedSequence[1][1]);
        EXPECT_FLOAT_EQ(11.0f / 25.0f, incrementedSequence[1][2]);
    }
    
    TEST(MATH_Random, FillHaltonSequence)
    {
        HaltonSequence<3> sequence({ 2, 3, 5 });
        auto regularSequence = sequence.GetHaltonSequence<5>();

        struct Point
        {
            Point() = default;
            Point(AZStd::array<float, 3> arr)
                :x(arr[0])
                ,y(arr[1])
                ,z(arr[2])
            {}

            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
        };

        AZStd::array<Point, 5> ownedContainer;
        sequence.FillHaltonSequence(ownedContainer.begin(), ownedContainer.end());

        for (size_t i = 0; i < regularSequence.size(); ++i)
        {
            EXPECT_FLOAT_EQ(regularSequence[i][0], ownedContainer[i].x);
            EXPECT_FLOAT_EQ(regularSequence[i][1], ownedContainer[i].y);
            EXPECT_FLOAT_EQ(regularSequence[i][2], ownedContainer[i].z);
        }
    }

}
