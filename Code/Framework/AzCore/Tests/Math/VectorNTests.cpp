/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/VectorN.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

namespace UnitTest
{
    class Math_VectorN
        : public UnitTest::LeakDetectionFixture
    {
    };

    TEST_F(Math_VectorN, TestConstructor)
    {
        AZ::VectorN v1(5, 5.0f);
        EXPECT_EQ(v1.GetDimensionality(), 5);
        for (AZStd::size_t iter = 0; iter < v1.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(v1.GetElement(iter), 5.0f);
        }
    }

    TEST_F(Math_VectorN, TestCreate)
    {
        AZ::VectorN zeros = AZ::VectorN::CreateZero(8);
        EXPECT_EQ(zeros.GetDimensionality(), 8);
        EXPECT_EQ(zeros.IsZero(), true);
        for (AZStd::size_t iter = 0; iter < zeros.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(zeros.GetElement(iter), 0.0f);
        }

        AZ::VectorN ones = AZ::VectorN::CreateOne(3);
        EXPECT_EQ(ones.GetDimensionality(), 3);
        for (AZStd::size_t iter = 0; iter < ones.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(ones.GetElement(iter), 1.0f);
        }

        AZ::VectorN random = AZ::VectorN::CreateRandom(1024);
        EXPECT_EQ(random.GetDimensionality(), 1024);
        for (AZStd::size_t iter = 0; iter < random.GetDimensionality(); ++iter)
        {
            ASSERT_GE(random.GetElement(iter), 0.0f);
            ASSERT_LE(random.GetElement(iter), 1.0f);
        }
    }

    TEST_F(Math_VectorN, TestNorms)
    {
        AZ::VectorN vec1 = AZ::VectorN::CreateZero(5);
        float vec1Length = vec1.L1Norm();
        EXPECT_FLOAT_EQ(vec1Length, 0.0f);
        vec1Length = vec1.L2Norm();
        EXPECT_FLOAT_EQ(vec1Length, 0.0f);

        AZ::VectorN vec2 = AZ::VectorN::CreateOne(16);
        float vec2Length = vec2.L1Norm();
        EXPECT_FLOAT_EQ(vec2Length, 16.0f);
        float vec2LengthSq = vec2.Dot(vec2);
        EXPECT_FLOAT_EQ(vec2LengthSq, 16.0f);
        vec2Length = vec2.L2Norm();
        EXPECT_FLOAT_EQ(vec2Length, 4.0f);

        AZ::VectorN vec3 = AZ::VectorN(4, 2.0f);
        float vec3Length = vec3.L1Norm();
        EXPECT_FLOAT_EQ(vec3Length, 8.0f);
        float vec3LengthSq = vec3.Dot(vec3);
        EXPECT_FLOAT_EQ(vec3LengthSq, 16.0f);
        vec3Length = vec3.L2Norm();
        EXPECT_FLOAT_EQ(vec3Length, 4.0f);
    }

    TEST_F(Math_VectorN, TestNormalize)
    {
        AZ::VectorN vec1 = AZ::VectorN(4, 4.0f);

        AZ::VectorN vec2 = vec1.GetNormalized();
        for (AZStd::size_t iter = 0; iter < vec2.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec2.GetElement(iter), 0.5f);
        }

        vec1.Normalize();
        for (AZStd::size_t iter = 0; iter < vec1.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec1.GetElement(iter), 0.5f);
        }
    }

    TEST_F(Math_VectorN, TestOperators)
    {
        AZ::VectorN vec1 = AZ::VectorN::CreateZero(5);
        for (AZStd::size_t iter = 0; iter < vec1.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec1.GetElement(iter), 0.0f);
        }

        AZ::VectorN vec2 = AZ::VectorN::CreateOne(5);
        for (AZStd::size_t iter = 0; iter < vec2.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec2.GetElement(iter), 1.0f);
        }

        AZ::VectorN vec3 = vec1 + vec2; // 0 + 1
        for (AZStd::size_t iter = 0; iter < vec3.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec3.GetElement(iter), 1.0f);
        }

        AZ::VectorN vec4 = vec3 - vec2; // 1 - 1
        for (AZStd::size_t iter = 0; iter < vec4.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec4.GetElement(iter), 0.0f);
        }

        AZ::VectorN vec5 = vec2 * 2.0f; // 1 * 2
        for (AZStd::size_t iter = 0; iter < vec5.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec5.GetElement(iter), 2.0f);
        }

        AZ::VectorN vec6 = vec5 * vec5; // 2 * 2
        for (AZStd::size_t iter = 0; iter < vec6.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec6.GetElement(iter), 4.0f);
        }

        AZ::VectorN vec7 = vec6 / vec5; // 4 / 2
        for (AZStd::size_t iter = 0; iter < vec7.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec7.GetElement(iter), 2.0f);
        }

        AZ::VectorN vec8 = vec7 / 2.0f; // 2 / 2
        for (AZStd::size_t iter = 0; iter < vec8.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec8.GetElement(iter), 1.0f);
        }

        AZ::VectorN vec9 = -vec6; // -4
        for (AZStd::size_t iter = 0; iter < vec9.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec9.GetElement(iter), -4.0f);
        }

        vec9 += vec6; // -4 + 4
        for (AZStd::size_t iter = 0; iter < vec9.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec9.GetElement(iter), 0.0f);
        }

        vec9 -= vec6; // 0 - 4
        for (AZStd::size_t iter = 0; iter < vec9.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec9.GetElement(iter), -4.0f);
        }

        vec9 *= 2.0f; // -4 * 2
        for (AZStd::size_t iter = 0; iter < vec9.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec9.GetElement(iter), -8.0f);
        }

        vec9 /= 2.0f; // -8 / 2
        for (AZStd::size_t iter = 0; iter < vec9.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec9.GetElement(iter), -4.0f);
        }

        vec9 *= vec5; // -4 * 2
        for (AZStd::size_t iter = 0; iter < vec9.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec9.GetElement(iter), -8.0f);
        }

        vec9 /= vec5; // -8 / 2
        for (AZStd::size_t iter = 0; iter < vec9.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec9.GetElement(iter), -4.0f);
        }

        vec9 = 1.0f - vec5; // 1 - 2
        for (AZStd::size_t iter = 0; iter < vec9.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec9.GetElement(iter), -1.0f);
        }

        vec9 = 1.0f + vec5; // 1 + 2
        for (AZStd::size_t iter = 0; iter < vec9.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec9.GetElement(iter), 3.0f);
        }

        vec9 = 2.0f * vec5; // 2 * 2
        for (AZStd::size_t iter = 0; iter < vec9.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(vec9.GetElement(iter), 4.0f);
        }
    }

    TEST_F(Math_VectorN, TestComparisons)
    {
        AZ::VectorN vec1 = AZ::VectorN(4, 4.0f);
        AZ::VectorN vec2 = AZ::VectorN(4, 4.1f);

        EXPECT_TRUE(vec1.IsClose(vec2, 0.125f));
        EXPECT_FALSE(vec1.IsClose(vec2, 0.0125f));

        EXPECT_TRUE(vec1.IsLessThan(vec2));
        EXPECT_FALSE(vec2.IsLessThan(vec1));
        EXPECT_TRUE(vec1.IsLessEqualThan(vec1));
        EXPECT_FALSE(vec2.IsLessEqualThan(vec1));

        EXPECT_FALSE(vec1.IsGreaterThan(vec2));
        EXPECT_TRUE(vec2.IsGreaterThan(vec1));
        EXPECT_TRUE(vec1.IsGreaterEqualThan(vec1));
        EXPECT_FALSE(vec1.IsGreaterEqualThan(vec2));
    }

    TEST_F(Math_VectorN, TestFloorCeilRound)
    {
        AZ::VectorN vec1 = AZ::VectorN(4, 4.1f);
        AZ::VectorN floor = vec1.GetFloor();
        for (AZStd::size_t iter = 0; iter < floor.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(floor.GetElement(iter), 4.0f);
        }

        AZ::VectorN ceil = vec1.GetCeil();
        for (AZStd::size_t iter = 0; iter < ceil.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(ceil.GetElement(iter), 5.0f);
        }

        AZ::VectorN round = vec1.GetRound();
        for (AZStd::size_t iter = 0; iter < round.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(round.GetElement(iter), 4.0f);
        }

        AZ::VectorN vec2 = AZ::VectorN(4, 4.5f);
        AZ::VectorN tie = vec2.GetRound();
        for (AZStd::size_t iter = 0; iter < tie.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(tie.GetElement(iter), 4.0f);
        }
    }

    TEST_F(Math_VectorN, TestMinMaxClamp)
    {
        AZ::VectorN random = AZ::VectorN::CreateRandom(7); // random vector between 0 and 1

        // min should be between 0.0 and 0.5
        AZ::VectorN min = random.GetMin(AZ::VectorN(7, 0.5f));
        for (AZStd::size_t iter = 0; iter < min.GetDimensionality(); ++iter)
        {
            EXPECT_LE(min.GetElement(iter), 0.5f);
        }

        // max should be between 0.5 and 1.0
        AZ::VectorN max = random.GetMax(AZ::VectorN(7, 0.5f));
        for (AZStd::size_t iter = 0; iter < max.GetDimensionality(); ++iter)
        {
            EXPECT_GE(max.GetElement(iter), 0.5f);
        }

        // clamp should be between 0.1 and 0.9
        AZ::VectorN clamp = random.GetClamp(AZ::VectorN(7, 0.1f), AZ::VectorN(7, 0.9f));
        for (AZStd::size_t iter = 0; iter < clamp.GetDimensionality(); ++iter)
        {
            EXPECT_GE(clamp.GetElement(iter), 0.1f);
            EXPECT_LE(clamp.GetElement(iter), 0.9f);
        }
    }

    TEST_F(Math_VectorN, TestSquareAbs)
    {
        AZ::VectorN random = AZ::VectorN::CreateRandom(200); // random vector between 0 and 1
        random -= 0.5f; // random vector between -0.5 and 0.5

        // absRand should be between 0.0 and 0.5
        AZ::VectorN absRand = random.GetAbs();
        for (AZStd::size_t iter = 0; iter < absRand.GetDimensionality(); ++iter)
        {
            EXPECT_GE(absRand.GetElement(iter), 0.0f);
            EXPECT_LE(absRand.GetElement(iter), 0.5f);
        }

        // sqRand should be between -0.25 and 0.25
        AZ::VectorN sqRand = random.GetSquare();
        for (AZStd::size_t iter = 0; iter < sqRand.GetDimensionality(); ++iter)
        {
            EXPECT_GE(sqRand.GetElement(iter), -0.25f);
            EXPECT_LE(sqRand.GetElement(iter),  0.25f);
        }
    }

    TEST_F(Math_VectorN, TestDot)
    {
        AZ::VectorN zeros = AZ::VectorN::CreateZero(15);
        AZ::VectorN ones = AZ::VectorN::CreateOne(15);

        float zerosDotOnes = zeros.Dot(ones);
        EXPECT_FLOAT_EQ(zerosDotOnes, 0.0f);

        float onesDotOnes = ones.Dot(ones);
        EXPECT_FLOAT_EQ(onesDotOnes, 15.0f);
    }
}
