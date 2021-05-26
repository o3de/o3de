/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    TEST(MATH_Random, HaltonSequence)
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

        sequence.SetOffsets({ 1, 2, 3 });
        auto offsetSequence = sequence.GetHaltonSequence<2>();

        EXPECT_FLOAT_EQ(1.0f / 4.0f, offsetSequence[0][0]);
        EXPECT_FLOAT_EQ(1.0f / 9.0f, offsetSequence[0][1]);
        EXPECT_FLOAT_EQ(4.0f / 5.0f, offsetSequence[0][2]);

        EXPECT_FLOAT_EQ(3.0f / 4.0f, offsetSequence[1][0]);
        EXPECT_FLOAT_EQ(4.0f / 9.0f, offsetSequence[1][1]);
        EXPECT_FLOAT_EQ(1.0f / 25.0f, offsetSequence[1][2]);
        
        sequence.SetIncrements({ 1, 2, 3 });
        auto incrementedSequence = sequence.GetHaltonSequence<2>();
        
        EXPECT_FLOAT_EQ(1.0f / 4.0f, incrementedSequence[0][0]);
        EXPECT_FLOAT_EQ(1.0f / 9.0f, incrementedSequence[0][1]);
        EXPECT_FLOAT_EQ(4.0f / 5.0f, incrementedSequence[0][2]);

        EXPECT_FLOAT_EQ(3.0f / 4.0f, incrementedSequence[1][0]);
        EXPECT_FLOAT_EQ(7.0f / 9.0f, incrementedSequence[1][1]);
        EXPECT_FLOAT_EQ(11.0f / 25.0f, incrementedSequence[1][2]);
    }
}
