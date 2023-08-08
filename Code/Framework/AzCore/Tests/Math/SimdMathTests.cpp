/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Internal/MathTypes.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    template<typename VectorType>
    void TestToVec1()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float testStoreValues[4] = { -1.0f, -1.0f, -1.0f, -1.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        Simd::Vec1::FloatType testVec1 = VectorType::ToVec1(testVector);
        Simd::Vec1::StoreUnaligned(testStoreValues, testVec1);

        for (int32_t i = 0; i < Simd::Vec1::ElementCount; ++i)
        {
            EXPECT_NEAR(testLoadValues[i], testStoreValues[i], AZ::Constants::Tolerance);
        }
    }

    template<typename VectorType>
    void TestFromVec1()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float testStoreValues[4] = { -1.0f, -1.0f, -1.0f, -1.0f };

        Simd::Vec1::FloatType testVec1 = Simd::Vec1::LoadUnaligned(testLoadValues);
        typename VectorType::FloatType testVector = VectorType::FromVec1(testVec1);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        // All the elements must be set to the Vec1.x value
        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_NEAR(testLoadValues[0], testStoreValues[i], AZ::Constants::Tolerance);
        }
    }

    template<typename VectorType>
    void TestToVec2()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float testStoreValues[4] = { -1.0f, -1.0f, -1.0f, -1.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        Simd::Vec2::FloatType testVec2 = VectorType::ToVec2(testVector);
        Simd::Vec2::StoreUnaligned(testStoreValues, testVec2);

        for (int32_t i = 0; i < Simd::Vec2::ElementCount; ++i)
        {
            EXPECT_NEAR(testLoadValues[i], testStoreValues[i], AZ::Constants::Tolerance);
        }
    }

    template<typename VectorType>
    void TestFromVec2()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float testStoreValues[4] = { -1.0f, -1.0f, -1.0f, -1.0f };

        Simd::Vec2::FloatType testVec2 = Simd::Vec2::LoadUnaligned(testLoadValues);
        typename VectorType::FloatType testVector = VectorType::FromVec2(testVec2);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        // The first 2 elements must be set to (Vec2.x, Vec2.y)
        for (int32_t i = 0; i < Simd::Vec2::ElementCount; ++i)
        {
            EXPECT_NEAR(testLoadValues[i], testStoreValues[i], AZ::Constants::Tolerance);
        }
        // The rest of elements must be set to zero
        for (int32_t i = Simd::Vec2::ElementCount; i < VectorType::ElementCount; ++i)
        {
            EXPECT_NEAR(0.0f, testStoreValues[i], AZ::Constants::Tolerance);
        }
    }

    template <typename VectorType>
    void TestLoadStoreFloat()
    {
        float testLoadValues[4]  = { 1.0f, 2.0f, 3.0f, 4.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_NEAR(testLoadValues[i], testStoreValues[i], AZ::Constants::Tolerance);
        }
    }

    template <typename VectorType>
    void TestLoadStoreInt()
    {
        int32_t testLoadValues[4] = { 1, 2, 3, 4 };
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };

        typename VectorType::Int32Type testVector = VectorType::LoadUnaligned(testLoadValues);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_NEAR(testLoadValues[i], testStoreValues[i], AZ::Constants::Tolerance);
        }
    }

    template <typename VectorType>
    void TestSelectFirst()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        const float selectedValue = VectorType::SelectIndex0(testVector);

        EXPECT_NEAR(testLoadValues[0], selectedValue, AZ::Constants::Tolerance);
    }

    template <typename VectorType>
    void TestSelectSecond()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        const float selectedValue = VectorType::SelectIndex1(testVector);

        EXPECT_NEAR(testLoadValues[1], selectedValue, AZ::Constants::Tolerance);
    }

    template <typename VectorType>
    void TestSelectThird()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        const float selectedValue = VectorType::SelectIndex2(testVector);

        EXPECT_NEAR(testLoadValues[2], selectedValue, AZ::Constants::Tolerance);
    }

    template <typename VectorType>
    void TestSelectFourth()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        const float selectedValue = VectorType::SelectIndex3(testVector);

        EXPECT_NEAR(testLoadValues[3], selectedValue, AZ::Constants::Tolerance);
    }

    template <typename VectorType>
    void ValidateReplaceResults(const float* testLoadValues1, const float* testLoadValues2, typename VectorType::FloatArgType result, uint32_t replaceIndex)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        VectorType::StoreUnaligned(testStoreValues, result);

        for (uint32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            if (i == replaceIndex)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(testLoadValues2[i], AZ::Constants::Tolerance));
            }
            else
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(testLoadValues1[i], AZ::Constants::Tolerance));
            }
        }
    }

    template <typename VectorType>
    void TestReplaceFirst()
    {
        float testLoadValues1[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        const typename VectorType::FloatType testVector1 = VectorType::LoadUnaligned(testLoadValues1);
        float testLoadValues2[4] = { -1.0f, -2.0f, -3.0f, -4.0f };
        const typename VectorType::FloatType testVector2 = VectorType::LoadUnaligned(testLoadValues2);

        typename VectorType::FloatType result1 = VectorType::ReplaceFirst(testVector1, testVector2);
        ValidateReplaceResults<VectorType>(testLoadValues1, testLoadValues2, result1, 0);

        typename VectorType::FloatType result2 = VectorType::ReplaceFirst(testVector1, testLoadValues2[0]);
        ValidateReplaceResults<VectorType>(testLoadValues1, testLoadValues2, result2, 0);
    }

    template <typename VectorType>
    void TestReplaceSecond()
    {
        float testLoadValues1[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        const typename VectorType::FloatType testVector1 = VectorType::LoadUnaligned(testLoadValues1);
        float testLoadValues2[4] = { -1.0f, -2.0f, -3.0f, -4.0f };
        const typename VectorType::FloatType testVector2 = VectorType::LoadUnaligned(testLoadValues2);

        typename VectorType::FloatType result1 = VectorType::ReplaceSecond(testVector1, testVector2);
        ValidateReplaceResults<VectorType>(testLoadValues1, testLoadValues2, result1, 1);

        typename VectorType::FloatType result2 = VectorType::ReplaceSecond(testVector1, testLoadValues2[1]);
        ValidateReplaceResults<VectorType>(testLoadValues1, testLoadValues2, result2, 1);
    }

    template <typename VectorType>
    void TestReplaceThird()
    {
        float testLoadValues1[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        const typename VectorType::FloatType testVector1 = VectorType::LoadUnaligned(testLoadValues1);
        float testLoadValues2[4] = { -1.0f, -2.0f, -3.0f, -4.0f };
        const typename VectorType::FloatType testVector2 = VectorType::LoadUnaligned(testLoadValues2);

        typename VectorType::FloatType result1 = VectorType::ReplaceThird(testVector1, testVector2);
        ValidateReplaceResults<VectorType>(testLoadValues1, testLoadValues2, result1, 2);

        typename VectorType::FloatType result2 = VectorType::ReplaceThird(testVector1, testLoadValues2[2]);
        ValidateReplaceResults<VectorType>(testLoadValues1, testLoadValues2, result2, 2);
    }

    template <typename VectorType>
    void TestReplaceFourth()
    {
        float testLoadValues1[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        const typename VectorType::FloatType testVector1 = VectorType::LoadUnaligned(testLoadValues1);
        float testLoadValues2[4] = { -1.0f, -2.0f, -3.0f, -4.0f };
        const typename VectorType::FloatType testVector2 = VectorType::LoadUnaligned(testLoadValues2);

        typename VectorType::FloatType result1 = VectorType::ReplaceFourth(testVector1, testVector2);
        ValidateReplaceResults<VectorType>(testLoadValues1, testLoadValues2, result1, 3);

        typename VectorType::FloatType result2 = VectorType::ReplaceFourth(testVector1, testLoadValues2[3]);
        ValidateReplaceResults<VectorType>(testLoadValues1, testLoadValues2, result2, 3);
    }

    template <typename VectorType>
    void TestSplatFloat()
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType testVector = VectorType::Splat(1.0f);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(1.0f, AZ::Constants::Tolerance));
        }
    }

    template <typename VectorType>
    void TestSplatInt()
    {
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };

        typename VectorType::Int32Type testVector = VectorType::Splat(1);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::Eq(1));
        }
    }

    template<typename VectorType>
    void TestSplatFirst()
    {
        float testLoadValues[4] = { 3.5f, 0.0f, 0.0f, 0.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        testVector = VectorType::SplatIndex0(testVector);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(3.5f, AZ::Constants::Tolerance));
        }
    }

    template<typename VectorType>
    void TestSplatSecond()
    {
        float testLoadValues[4] = { 0.0f, 3.5f, 0.0f, 0.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        testVector = VectorType::SplatIndex1(testVector);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(3.5f, AZ::Constants::Tolerance));
        }
    }

    template<typename VectorType>
    void TestSplatThird()
    {
        float testLoadValues[4] = { 0.0f, 0.0f, 3.5f, 0.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        testVector = VectorType::SplatIndex2(testVector);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(3.5f, AZ::Constants::Tolerance));
        }
    }

    template<typename VectorType>
    void TestSplatFourth()
    {
        float testLoadValues[4] = { 0.0f, 0.0f, 0.0f, 3.5f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        testVector = VectorType::SplatIndex3(testVector);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(3.5f, AZ::Constants::Tolerance));
        }
    }

    template <typename VectorType>
    void TestLoadImmediateFloat()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType testVector = VectorType::LoadUnaligned(testLoadValues);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(testLoadValues[i], AZ::Constants::Tolerance));
        }
    }

    template <typename VectorType>
    void TestLoadImmediateInt()
    {
        int32_t testLoadValues[4] = { 1, 2, 3, 4 };
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };

        typename VectorType::Int32Type testVector = VectorType::LoadUnaligned(testLoadValues);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::Eq(testLoadValues[i]));
        }
    }

    template <typename VectorType>
    void TestAddSubMulDivFloat()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);

        // Add
        {
            float results[4] = { 2.0f, 4.0f, 6.0f, 8.0f };
            typename VectorType::FloatType testVector = VectorType::Add(sourceVector, sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Sub
        {
            float results[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
            typename VectorType::FloatType testVector = VectorType::Sub(VectorType::Add(sourceVector, sourceVector), sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Mul
        {
            float results[4] = { 1.0f, 4.0f, 9.0f, 16.0f };
            typename VectorType::FloatType testVector = VectorType::Mul(sourceVector, sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Div
        {
            float results[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            typename VectorType::FloatType testVector = VectorType::Div(sourceVector, sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }
    }

    template <typename VectorType>
    void TestAddSubMulInt()
    {
        int32_t testLoadValues[4] = { 1, 2, 3, 4 };
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };

        typename VectorType::Int32Type sourceVector = VectorType::LoadUnaligned(testLoadValues);

        // Add
        {
            int32_t results[4] = { 2, 4, 6, 8 };
            typename VectorType::Int32Type testVector = VectorType::Add(sourceVector, sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // Sub
        {
            int32_t results[4] = { 1, 2, 3, 4 };
            typename VectorType::Int32Type testVector = VectorType::Sub(VectorType::Add(sourceVector, sourceVector), sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // Mul
        {
            int32_t results[4] = { 1, 4, 9, 16 };
            typename VectorType::Int32Type testVector = VectorType::Mul(sourceVector, sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }
    }

    template <typename VectorType>
    void TestNotFloat()
    {
        float testLoadValues[4] = { 0.0f, 1.0f, -0.0f, -1.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);
        typename VectorType::FloatType testVector = VectorType::Not(sourceVector);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        switch (VectorType::ElementCount)
        {
        case 4:
            EXPECT_NEAR(testStoreValues[3], 4.0f, 0.01f);
        case 3:
            EXPECT_TRUE(AZStd::isnan(testStoreValues[2]));
        case 2:
            EXPECT_NEAR(testStoreValues[1], -4.0f, 0.01f);
        case 1:
            EXPECT_TRUE(AZStd::isnan(testStoreValues[0]));
            break;
        }
    }

    template <typename VectorType>
    void TestAndAndnotOrFloat()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, -3.0f, -4.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);

        // And
        {
            float results[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            typename VectorType::FloatType testVector = VectorType::And(sourceVector, VectorType::ZeroFloat());
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // AndNot
        {
            typename VectorType::FloatType testVector = VectorType::AndNot(VectorType::ZeroFloat(), sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(testLoadValues[i], AZ::Constants::Tolerance));
            }
        }

        // Or
        {
            float results[4] = { 1.0f, 2.0f, -3.0f, -4.0f };
            typename VectorType::FloatType testVector = VectorType::Or(sourceVector, VectorType::ZeroFloat());
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }
    }

    template <typename VectorType>
    void TestXorFloat()
    {
        float testLoadValues[4] = { 1.0f, 2.0f, -3.0f, -4.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::Int32Type mask = VectorType::Splat(static_cast<int32_t>(0x80000000));
        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);
        typename VectorType::FloatType testVector = VectorType::Xor(sourceVector, VectorType::CastToFloat(mask));
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(-testLoadValues[i], AZ::Constants::Tolerance));
        }
    }

    template <typename VectorType>
    void TestNotInt()
    {
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };

        typename VectorType::Int32Type sourceVector = VectorType::Splat(static_cast<int32_t>(0x80000000));
        typename VectorType::Int32Type testVector = VectorType::Not(sourceVector);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::Eq(0x7FFFFFFF));
        }
    }

    template <typename VectorType>
    void TestAndAndnotOrInt()
    {
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };

        typename VectorType::Int32Type sourceVector1 = VectorType::Splat(static_cast<int32_t>(0xFFFF0000));
        typename VectorType::Int32Type sourceVector2 = VectorType::Splat(static_cast<int32_t>(0x0000FFFF));

        // And
        {
            typename VectorType::Int32Type testVector = VectorType::And(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(0));
            }
        }

        // AndNot
        {
            typename VectorType::Int32Type testVector = VectorType::AndNot(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(0x0000FFFF));
            }
        }

        // Or
        {
            typename VectorType::Int32Type testVector = VectorType::Or(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(0xFFFFFFFF));
            }
        }
    }

    template <typename VectorType>
    void TestXorInt()
    {
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };

        typename VectorType::Int32Type sourceVector1 = VectorType::Splat(static_cast<int32_t>(0xFFFFF000));
        typename VectorType::Int32Type sourceVector2 = VectorType::Splat(static_cast<int32_t>(0x000FFFFF));
        typename VectorType::Int32Type testVector = VectorType::Xor(sourceVector1, sourceVector2);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::Eq(0xFFF00FFF));
        }
    }

    template <typename VectorType>
    void TestMinMaxInt()
    {
        int32_t testLoadValues1[4] = {  0,  1, -1,  0 };
        int32_t testLoadValues2[4] = { -1,  0,  0,  1 };
        int32_t testStoreValues[4] = {  0,  0,  0,  0 };

        typename VectorType::Int32Type sourceVector1 = VectorType::LoadUnaligned(testLoadValues1);
        typename VectorType::Int32Type sourceVector2 = VectorType::LoadUnaligned(testLoadValues2);

        // Min
        {
            int32_t results[4] = { -1, 0, -1, 0 };
            typename VectorType::Int32Type testVector = VectorType::Min(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // Max
        {
            int32_t results[4] = { 0, 1, 0, 1 };
            typename VectorType::Int32Type testVector = VectorType::Max(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }
    }

    template <typename VectorType>
    void TestClampInt()
    {
        int32_t testLoadValues[4] = { -10, 10, -5, 5 };
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };

        typename VectorType::Int32Type sourceVector1 = VectorType::LoadUnaligned(testLoadValues);
        typename VectorType::Int32Type sourceVector2 = VectorType::Splat(-8);
        typename VectorType::Int32Type sourceVector3 = VectorType::Splat( 8);
        typename VectorType::Int32Type testVector = VectorType::Clamp(sourceVector1, sourceVector2, sourceVector3);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        int32_t results[4] = { -8, 8, -5, 5 };
        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
        }
    }

    template <typename VectorType>
    void TestFloorFloat()
    {
        // Positive values
        float testLoadValues[4] = { 0.2f, 0.4f, 0.6f, 0.8f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Floor(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(0.0f, AZ::Constants::Tolerance));
            }
        }

        // Negative values
        float testNegativeLoadValues[4] = { -0.2f, -0.4f, -0.6f, -0.8f };
        sourceVector = VectorType::LoadUnaligned(testNegativeLoadValues);

        {
            typename VectorType::FloatType testVector = VectorType::Floor(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(-1.0f, AZ::Constants::Tolerance));
            }
        }

        // Equidistant values
        float testEquidistantLoadValues[4] = { 0.5f, -0.5f, 1.5f, -1.5f };
        sourceVector = VectorType::LoadUnaligned(testEquidistantLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Floor(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { 0.0f, -1.0f, 1.0f, -2.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Whole values
        float testWholeLoadValues[4] = { 0.0f, 1.0f, -1.0f, 2.0f };
        sourceVector = VectorType::LoadUnaligned(testWholeLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Floor(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(testWholeLoadValues[i], AZ::Constants::Tolerance));
            }
        }
    }

    template <typename VectorType>
    void TestCeilFloat()
    {
        // Positive values
        float testLoadValues[4] = { 0.2f, 0.4f, 0.6f, 0.8f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Ceil(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(1.0f, AZ::Constants::Tolerance));
            }
        }

        // Negative values
        float testNegativeLoadValues[4] = { -0.2f, -0.4f, -0.6f, -0.8f };
        sourceVector = VectorType::LoadUnaligned(testNegativeLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Ceil(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(0.0f, AZ::Constants::Tolerance));
            }
        }

        // Equidistant values
        float testEquidistantLoadValues[4] = { 0.5f, -0.5f, 1.5f, -1.5f };
        sourceVector = VectorType::LoadUnaligned(testEquidistantLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Ceil(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { 1.0f, 0.0f, 2.0f, -1.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Whole values
        float testWholeLoadValues[4] = { 0.0f, 1.0f, -1.0f, 2.0f };
        sourceVector = VectorType::LoadUnaligned(testWholeLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Ceil(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(testWholeLoadValues[i], AZ::Constants::Tolerance));
            }
        }
    }

    template <typename VectorType>
    void TestRoundFloat()
    {
        // Positive values
        float testLoadValues[4] = { 0.2f, 0.4f, 0.6f, 0.8f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Round(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Negative values
        float testNegativeLoadValues[4] = { -0.2f, -0.4f, -0.6f, -0.8f };
        sourceVector = VectorType::LoadUnaligned(testNegativeLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Round(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { 0.0f, 0.0f, -1.0f, -1.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Equidistant values
        float testEquidistantLoadValues[4] = { 0.5f, -0.5f, 1.5f, -1.5f };
        sourceVector = VectorType::LoadUnaligned(testEquidistantLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Round(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            // The expected behaviour is ties to even (banker's rounding)
            float results[4] = { 0.0f, 0.0f, 2.0f, -2.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Whole values
        float testWholeLoadValues[4] = { 0.0f, 1.0f, -1.0f, 2.0f };
        sourceVector = VectorType::LoadUnaligned(testWholeLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Round(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(testWholeLoadValues[i], AZ::Constants::Tolerance));
            }
        }
    }

    template <typename VectorType>
    void TestTruncateFloat()
    {
        // Positive values
        float testLoadValues[4] = { 0.2f, 0.4f, 0.6f, 0.8f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Truncate(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Negative values
        float testNegativeLoadValues[4] = { -0.2f, -0.4f, -0.6f, -0.8f };
        sourceVector = VectorType::LoadUnaligned(testNegativeLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Truncate(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Equidistant values
        float testEquidistantLoadValues[4] = { 0.5f, -0.5f, 1.5f, -1.5f };
        sourceVector = VectorType::LoadUnaligned(testEquidistantLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Truncate(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { 0.0f, 0.0f, 1.0f, -1.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Whole values
        float testWholeLoadValues[4] = { 0.0f, 1.0f, -1.0f, 2.0f };
        sourceVector = VectorType::LoadUnaligned(testWholeLoadValues);
        {
            typename VectorType::FloatType testVector = VectorType::Truncate(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(testWholeLoadValues[i], AZ::Constants::Tolerance));
            }
        }
    }

    template <typename VectorType>
    void TestMinMaxFloat()
    {
        float testLoadValues1[4] = {  0.0f,  1.0f, -1.0f,  0.0f };
        float testLoadValues2[4] = { -1.0f,  0.0f,  0.0f,  1.0f };
        float testStoreValues[4] = {  0.0f,  0.0f,  0.0f,  0.0f };

        typename VectorType::FloatType sourceVector1 = VectorType::LoadUnaligned(testLoadValues1);
        typename VectorType::FloatType sourceVector2 = VectorType::LoadUnaligned(testLoadValues2);

        // Min
        {
            typename VectorType::FloatType testVector = VectorType::Min(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { -1.0f, 0.0f, -1.0f, 0.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }

        // Max
        {
            typename VectorType::FloatType testVector = VectorType::Max(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
            }
        }
    }

    template <typename VectorType>
    void TestClampFloat()
    {
        float testLoadValues[4] = { -10.0f, 10.0f, -5.0f, 5.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType sourceVector1 = VectorType::LoadUnaligned(testLoadValues);
        typename VectorType::FloatType sourceVector2 = VectorType::Splat(-8.0f);
        typename VectorType::FloatType sourceVector3 = VectorType::Splat(8.0f);
        typename VectorType::FloatType testVector = VectorType::Clamp(sourceVector1, sourceVector2, sourceVector3);
        VectorType::StoreUnaligned(testStoreValues, testVector);

        float results[4] = { -8.0f, 8.0f, -5.0f, 5.0f };
        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(results[i], AZ::Constants::Tolerance));
        }
    }

    template <typename VectorType>
    void TestCompareFloat()
    {
        float testLoadValues1[4] = { -10.1f, 10.0f, -5.0f, 5.1f };
        float testLoadValues2[4] = { -10.0f,  0.0f, -5.0f, 5.1f };
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };

        typename VectorType::FloatType sourceVector1 = VectorType::LoadUnaligned(testLoadValues1);
        typename VectorType::FloatType sourceVector2 = VectorType::LoadUnaligned(testLoadValues2);

        // CmpEq
        {
            typename VectorType::FloatType testVector = VectorType::CmpEq(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, VectorType::CastToInt(testVector));

            int32_t results[4] = { 0, 0, -1, -1 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // CmpNeq
        {
            typename VectorType::FloatType testVector = VectorType::CmpNeq(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, VectorType::CastToInt(testVector));

            int32_t results[4] = { -1, -1, 0, 0 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // CmpGt
        {
            typename VectorType::FloatType testVector = VectorType::CmpGt(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, VectorType::CastToInt(testVector));

            int32_t results[4] = { 0, -1, 0, 0 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // CmpGte
        {
            typename VectorType::FloatType testVector = VectorType::CmpGtEq(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, VectorType::CastToInt(testVector));

            int32_t results[4] = { 0, -1, -1, -1 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // CmpLt
        {
            typename VectorType::FloatType testVector = VectorType::CmpLt(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, VectorType::CastToInt(testVector));

            int32_t results[4] = { -1, 0, 0, 0 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // CmpLte
        {
            typename VectorType::FloatType testVector = VectorType::CmpLtEq(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, VectorType::CastToInt(testVector));

            int32_t results[4] = { -1, 0, -1, -1 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }
    }

    template <typename VectorType>
    void TestCompareAllFloat()
    {
        float testLoadValues1[4] = { -10.1f, 10.0f, -5.1f, 5.0f };
        float testLoadValues2[4] = { -10.0f, 10.1f, -5.0f, 5.1f };

        typename VectorType::FloatType sourceVector1 = VectorType::LoadUnaligned(testLoadValues1);
        typename VectorType::FloatType sourceVector2 = VectorType::LoadUnaligned(testLoadValues2);

        EXPECT_FALSE(VectorType::CmpAllEq(sourceVector1, sourceVector2));
        EXPECT_TRUE (VectorType::CmpAllLt(sourceVector1, sourceVector2));
        EXPECT_TRUE (VectorType::CmpAllLtEq(sourceVector1, sourceVector2));
        EXPECT_FALSE(VectorType::CmpAllGt(sourceVector1, sourceVector2));
        EXPECT_FALSE(VectorType::CmpAllGtEq(sourceVector1, sourceVector2));

        EXPECT_TRUE(VectorType::CmpAllEq(sourceVector1, sourceVector1));
        EXPECT_TRUE(VectorType::CmpAllEq(sourceVector2, sourceVector2));

        EXPECT_FALSE(VectorType::CmpAllLt(sourceVector1, sourceVector1));
        EXPECT_FALSE(VectorType::CmpAllGt(sourceVector1, sourceVector1));
        EXPECT_FALSE(VectorType::CmpAllLt(sourceVector2, sourceVector2));
        EXPECT_FALSE(VectorType::CmpAllGt(sourceVector2, sourceVector2));

        EXPECT_TRUE(VectorType::CmpAllLtEq(sourceVector1, sourceVector1));
        EXPECT_TRUE(VectorType::CmpAllGtEq(sourceVector1, sourceVector1));
        EXPECT_TRUE(VectorType::CmpAllLtEq(sourceVector2, sourceVector2));
        EXPECT_TRUE(VectorType::CmpAllGtEq(sourceVector2, sourceVector2));
    }

    template <typename VectorType>
    void TestCompareInt()
    {
        int32_t testLoadValues1[4] = { -11, 10, -5, 6 };
        int32_t testLoadValues2[4] = { -10,  0, -5, 6 };
        int32_t testStoreValues[4] = {   0,  0,  0, 0 };

        typename VectorType::Int32Type sourceVector1 = VectorType::LoadUnaligned(testLoadValues1);
        typename VectorType::Int32Type sourceVector2 = VectorType::LoadUnaligned(testLoadValues2);

        // CmpEq
        {
            typename VectorType::Int32Type testVector = VectorType::CmpEq(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { 0, 0, -1, -1 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // CmpNeq
        {
            typename VectorType::Int32Type testVector = VectorType::CmpNeq(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { -1, -1, 0, 0 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // CmpGt
        {
            typename VectorType::Int32Type testVector = VectorType::CmpGt(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { 0, -1, 0, 0 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // CmpGte
        {
            typename VectorType::Int32Type testVector = VectorType::CmpGtEq(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { 0, -1, -1, -1 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // CmpLt
        {
            typename VectorType::Int32Type testVector = VectorType::CmpLt(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { -1, 0, 0, 0 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // CmpLte
        {
            typename VectorType::Int32Type testVector = VectorType::CmpLtEq(sourceVector1, sourceVector2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { -1, 0, -1, -1 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }
    }

    template <typename VectorType>
    void TestCompareAllInt()
    {
        int32_t testLoadValues1[4] = { -10, 11, -4, 6 };
        int32_t testLoadValues2[4] = { -11, 10, -5, 5 };

        typename VectorType::Int32Type sourceVector1 = VectorType::LoadUnaligned(testLoadValues1);
        typename VectorType::Int32Type sourceVector2 = VectorType::LoadUnaligned(testLoadValues2);

        EXPECT_FALSE(VectorType::CmpAllEq(sourceVector1, sourceVector2));
        EXPECT_TRUE(VectorType::CmpAllEq(sourceVector1, sourceVector1));
        EXPECT_TRUE(VectorType::CmpAllEq(sourceVector2, sourceVector2));
    }

    template <typename VectorType>
    void TestReciprocalFloat()
    {
        float testLoadValues[4] = { 2.0f, 4.0f, 8.0f, 1.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);

        // Reciprocal
        {
            typename VectorType::FloatType testVector = VectorType::Reciprocal(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { 0.5f, 0.25f, 0.125f, 1.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_NEAR(testStoreValues[i], results[i], 0.01f);
            }
        }

        // ReciprocalEstimate
        {
            typename VectorType::FloatType testVector = VectorType::ReciprocalEstimate(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            float results[4] = { 0.5f, 0.25f, 0.125f, 1.0f };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_NEAR(testStoreValues[i], results[i], 0.01f);
            }
        }
    }

    template <typename VectorType>
    void TestSqrtInvSqrtFloat()
    {
        float testLoadValues[4] = { -1.0f, 16.0f, 1.0f, 0.0f };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);

        // Sqrt
        {
            typename VectorType::FloatType testVector = VectorType::Sqrt(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues[3], 0.0f, 0.002f);
            case 3:
                EXPECT_NEAR(testStoreValues[2], 1.0f, 0.002f);
            case 2:
                EXPECT_NEAR(testStoreValues[1], 4.0f, 0.002f);
            case 1:
                EXPECT_TRUE(AZStd::isnan(testStoreValues[0]));
                break;
            }
        }

        // InvSqrt
        {
            typename VectorType::FloatType testVector = VectorType::SqrtInv(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_TRUE(AZStd::isnan(testStoreValues[3]) || AZStd::isinf(testStoreValues[3]));
            case 3:
                EXPECT_NEAR(testStoreValues[2], 1.0f, 0.002f);
            case 2:
                EXPECT_NEAR(testStoreValues[1], 0.25f, 0.002f);
            case 1:
                EXPECT_TRUE(AZStd::isnan(testStoreValues[0]));
                break;
            }
        }
    }

    template <typename VectorType>
    void TestSin()
    {
        float testLoadValues1[4] = { 0.0f, 0.523598775598f, 1.5707963267949f, 3.141594f };
        float testLoadValues2[4] = { 21.991148575f, 4.18879020479f, -1.5707963267949f, -3.1415926f };
        float testStoreValues1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float testStoreValues2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float precision = 0.000002f;

        typename VectorType::FloatType sourceVector1 = VectorType::LoadUnaligned(testLoadValues1);
        typename VectorType::FloatType sourceVector2 = VectorType::LoadUnaligned(testLoadValues2);

        {
            typename VectorType::FloatType testVector = VectorType::Sin(sourceVector1);
            VectorType::StoreUnaligned(testStoreValues1, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues1[3], 0.0f, precision);
            case 3:
                EXPECT_NEAR(testStoreValues1[2], 1.0f, precision);
            case 2:
                EXPECT_NEAR(testStoreValues1[1], 0.5f, precision);
            case 1:
                EXPECT_NEAR(testStoreValues1[0], 0.0f, precision);
            }
        }

        {
            typename VectorType::FloatType testVector = VectorType::Sin(sourceVector2);
            VectorType::StoreUnaligned(testStoreValues2, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues2[3], 0.0f, precision);
            case 3:
                EXPECT_NEAR(testStoreValues2[2], -1.0f, precision);
            case 2:
                EXPECT_NEAR(testStoreValues2[1], -0.8660254f, precision);
            case 1:
                EXPECT_NEAR(testStoreValues2[0], 0.0f, precision);
            }
        }
    }

    template <typename VectorType>
    void TestCos()
    {
        float testLoadValues1[4] = { 0.0f, 0.523598775598f, 1.5707963267949f, 3.141594f };
        float testLoadValues2[4] = { 21.991148575f, 4.18879020479f, -1.5707963267949f, -3.1415926f };
        float testStoreValues1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float testStoreValues2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float precision = 0.000002f;

        typename VectorType::FloatType sourceVector1 = VectorType::LoadUnaligned(testLoadValues1);
        typename VectorType::FloatType sourceVector2 = VectorType::LoadUnaligned(testLoadValues2);

        {
            typename VectorType::FloatType testVector = VectorType::Cos(sourceVector1);
            VectorType::StoreUnaligned(testStoreValues1, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues1[3], -1.0f, precision);
            case 3:
                EXPECT_NEAR(testStoreValues1[2], 0.0f, precision);
            case 2:
                EXPECT_NEAR(testStoreValues1[1], 0.8660254f, precision);
            case 1:
                EXPECT_NEAR(testStoreValues1[0], 1.0f, precision);
            }
        }

        {
            typename VectorType::FloatType testVector = VectorType::Cos(sourceVector2);
            VectorType::StoreUnaligned(testStoreValues2, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues2[3], -1.0f, precision);
            case 3:
                EXPECT_NEAR(testStoreValues2[2], 0.0f, precision);
            case 2:
                EXPECT_NEAR(testStoreValues2[1], -0.5f, precision);
            case 1:
                EXPECT_NEAR(testStoreValues2[0], -1.0f, precision);
            }
        }
    }

    template <typename VectorType>
    void TestAcos()
    {
        float testLoadValues1[4] = { -1.0f, -0.8660254f, -0.5, 0.0f };
        float testLoadValues2[4] = { 0.70710678f, 1.0f, -1.5f, 2.0f };
        float testStoreValues1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float testStoreValues2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float precision = 0.000002f;

        typename VectorType::FloatType sourceVector1 = VectorType::LoadUnaligned(testLoadValues1);
        typename VectorType::FloatType sourceVector2 = VectorType::LoadUnaligned(testLoadValues2);

        {
            typename VectorType::FloatType testVector = VectorType::Acos(sourceVector1);
            VectorType::StoreUnaligned(testStoreValues1, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues1[3], Constants::HalfPi, precision);
            case 3:
                EXPECT_NEAR(testStoreValues1[2], 2 * Constants::Pi / 3.0f, precision);
            case 2:
                EXPECT_NEAR(testStoreValues1[1], 5 * Constants::Pi / 6.0f, precision);
            case 1:
                EXPECT_NEAR(testStoreValues1[0], Constants::Pi, precision);
            }
        }

        {
            typename VectorType::FloatType testVector = VectorType::Acos(sourceVector2);
            VectorType::StoreUnaligned(testStoreValues2, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_TRUE(AZStd::isnan(testStoreValues2[3]));
            case 3:
                EXPECT_TRUE(AZStd::isnan(testStoreValues2[2]));
            case 2:
                EXPECT_NEAR(testStoreValues2[1], 0.0f, precision);
            case 1:
                EXPECT_NEAR(testStoreValues2[0], Constants::QuarterPi, precision);
            }
        }
    }

    template <typename VectorType>
    void TestAtan()
    {
        float testLoadValues1[4] = { 0.0f, 0.5773503f, 1.0f, 1000.0f };
        float testLoadValues2[4] = { 0.0f, -0.5773503f, -1.0f, -1000.0f };
        float testStoreValues1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float testStoreValues2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float precision = 0.000002f;

        typename VectorType::FloatType sourceVector1 = VectorType::LoadUnaligned(testLoadValues1);
        typename VectorType::FloatType sourceVector2 = VectorType::LoadUnaligned(testLoadValues2);

        {
            typename VectorType::FloatType testVector = VectorType::Atan(sourceVector1);
            VectorType::StoreUnaligned(testStoreValues1, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues1[3], 1.569796f, precision);
            case 3:
                EXPECT_NEAR(testStoreValues1[2], Constants::Pi / 4.0f, precision);
            case 2:
                EXPECT_NEAR(testStoreValues1[1], Constants::Pi / 6.0f, precision);
            case 1:
                EXPECT_NEAR(testStoreValues1[0], 0.0f, precision);
            }
        }

        {
            typename VectorType::FloatType testVector = VectorType::Atan(sourceVector2);
            VectorType::StoreUnaligned(testStoreValues2, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues2[3], -1.569796f, precision);
            case 3:
                EXPECT_NEAR(testStoreValues2[2], -Constants::Pi / 4.0f, precision);
            case 2:
                EXPECT_NEAR(testStoreValues2[1], -Constants::Pi / 6.0f, precision);
            case 1:
                EXPECT_NEAR(testStoreValues2[0], 0.0f, precision);
            }
        }
    }

    template <typename VectorType>
    void TestAtan2()
    {
        float testLoadValuesY1[4] = { 0.0f, 1.0f, -1.0f, 2.0f };
        float testLoadValuesX1[4] = { 0.0f, 0.0f, 0.0f, 2.0f };
        float testStoreValues1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        float testLoadValuesY2[4] = { 0.0f, 1.0f, -1.0f, 0.0f };
        float testLoadValuesX2[4] = { -1.0f, -1.0f, -1.0f, 1.0f };   
        float testStoreValues2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        float precision = 0.000002f;

        typename VectorType::FloatType sourceVectorY1 = VectorType::LoadUnaligned(testLoadValuesY1);
        typename VectorType::FloatType sourceVectorX1 = VectorType::LoadUnaligned(testLoadValuesX1);
        typename VectorType::FloatType sourceVectorY2 = VectorType::LoadUnaligned(testLoadValuesY2);
        typename VectorType::FloatType sourceVectorX2 = VectorType::LoadUnaligned(testLoadValuesX2);

        {
            typename VectorType::FloatType testVector = VectorType::Atan2(sourceVectorY1, sourceVectorX1);
            VectorType::StoreUnaligned(testStoreValues1, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues1[3], Constants::Pi / 4.0f, precision);
            case 3:
                EXPECT_NEAR(testStoreValues1[2], -Constants::HalfPi, precision);
            case 2:
                EXPECT_NEAR(testStoreValues1[1], Constants::HalfPi, precision);
            case 1:
                EXPECT_NEAR(testStoreValues1[0], 0.0f, precision);
            }
        }

        {
            typename VectorType::FloatType testVector = VectorType::Atan2(sourceVectorY2, sourceVectorX2);
            VectorType::StoreUnaligned(testStoreValues2, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues2[3], 0.0f, precision);
            case 3:
                EXPECT_NEAR(testStoreValues2[2], -3 * Constants::Pi / 4.0f, precision);
            case 2:
                EXPECT_NEAR(testStoreValues2[1], 3 * Constants::Pi / 4.0f, precision);
            case 1:
                EXPECT_NEAR(testStoreValues2[0], Constants::Pi, precision);
            }
        }
    }

    template <typename VectorType>
    void TestExpEstimate()
    {
        float testLoadValuesX1[4] = { 0.0f, 1.0f, -1.0f, 2.0f };
        float testLoadValuesX2[4] = { 0.5f, 0.9f, -1.1f, 9.0f };

        float precision = 0.005f;

        typename VectorType::FloatType sourceVectorX1 = VectorType::LoadUnaligned(testLoadValuesX1);
        typename VectorType::FloatType sourceVectorX2 = VectorType::LoadUnaligned(testLoadValuesX2);

        {
            float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            typename VectorType::FloatType testVector = VectorType::ExpEstimate(sourceVectorX1);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_TRUE(AZ::IsCloseMag(testStoreValues[3], exp(2.0f), precision));
            case 3:
                EXPECT_TRUE(AZ::IsCloseMag(testStoreValues[2], exp(-1.0f), precision));
            case 2:
                EXPECT_TRUE(AZ::IsCloseMag(testStoreValues[1], exp(1.0f), precision));
            case 1:
                EXPECT_TRUE(AZ::IsCloseMag(testStoreValues[0], exp(0.0f), precision));
            }
        }

        {
            float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            typename VectorType::FloatType testVector = VectorType::ExpEstimate(sourceVectorX2);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_TRUE(AZ::IsCloseMag(testStoreValues[3], exp(9.0f), precision));
            case 3:
                EXPECT_TRUE(AZ::IsCloseMag(testStoreValues[2], exp(-1.1f), precision));
            case 2:
                EXPECT_TRUE(AZ::IsCloseMag(testStoreValues[1], exp(0.9f), precision));
            case 1:
                EXPECT_TRUE(AZ::IsCloseMag(testStoreValues[0], exp(0.5f), precision));
            }
        }
    }

    template <typename VectorType>
    void TestConvertToInt()
    {
        // Positive values
        float testLoadValues[4] = { 0.2f, 0.4f, 0.6f, 0.8f };
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };
        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);
        {
            typename VectorType::Int32Type testVector = VectorType::ConvertToInt(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { 0, 0, 0, 0 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // Negative values
        float testNegativeLoadValues[4] = { -0.2f, -0.4f, -0.6f, -0.8f };
        sourceVector = VectorType::LoadUnaligned(testNegativeLoadValues);
        {
            typename VectorType::Int32Type testVector = VectorType::ConvertToInt(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { 0, 0, 0, 0 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // Equidistant values
        float testEquidistantLoadValues[4] = { 0.5f, -0.5f, 1.5f, -1.5f };
        sourceVector = VectorType::LoadUnaligned(testEquidistantLoadValues);
        {
            typename VectorType::Int32Type testVector = VectorType::ConvertToInt(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { 0, 0, 1, -1 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // Whole values
        float testWholeLoadValues[4] = { 0.0f, 1.0f, -1.0f, 2.0f };
        sourceVector = VectorType::LoadUnaligned(testWholeLoadValues);
        {
            typename VectorType::Int32Type testVector = VectorType::ConvertToInt(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { 0, 1, -1, 2 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }
    }

    template <typename VectorType>
    void TestConvertToIntNearest()
    {
        // Positive values
        float testLoadValues[4] = { 0.2f, 0.4f, 0.6f, 0.8f };
        int32_t testStoreValues[4] = { 0, 0, 0, 0 };
        typename VectorType::FloatType sourceVector = VectorType::LoadUnaligned(testLoadValues);
        {
            typename VectorType::Int32Type testVector = VectorType::ConvertToIntNearest(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { 0, 0, 1, 1 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // Negative values
        float testNegativeLoadValues[4] = { -0.2f, -0.4f, -0.6f, -0.8f };
        sourceVector = VectorType::LoadUnaligned(testNegativeLoadValues);
        {
            typename VectorType::Int32Type testVector = VectorType::ConvertToIntNearest(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { 0, 0, -1, -1 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // Equidistant values
        float testEquidistantLoadValues[4] = { 0.5f, -0.5f, 1.5f, -1.5f };
        sourceVector = VectorType::LoadUnaligned(testEquidistantLoadValues);
        {
            typename VectorType::Int32Type testVector = VectorType::ConvertToIntNearest(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            // The expected behaviour is ties to even (banker's rounding)
            int32_t results[4] = { 0, 0, 2, -2 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }

        // Whole values
        float testWholeLoadValues[4] = { 0.0f, 1.0f, -1.0f, 2.0f };
        sourceVector = VectorType::LoadUnaligned(testWholeLoadValues);
        {
            typename VectorType::Int32Type testVector = VectorType::ConvertToIntNearest(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            int32_t results[4] = { 0, 1, -1, 2 };
            for (int32_t i = 0; i < VectorType::ElementCount; ++i)
            {
                EXPECT_THAT(testStoreValues[i], ::testing::Eq(results[i]));
            }
        }
    }

    template <typename VectorType>
    void TestConvertToFloat()
    {
        int32_t testLoadValues[4] = { -1, 0, 2, 4 };
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        typename VectorType::Int32Type sourceVector = VectorType::LoadUnaligned(testLoadValues);

        {
            typename VectorType::FloatType testVector = VectorType::ConvertToFloat(sourceVector);
            VectorType::StoreUnaligned(testStoreValues, testVector);

            switch (VectorType::ElementCount)
            {
            case 4:
                EXPECT_NEAR(testStoreValues[3], 4.0f, 0.002f);
            case 3:
                EXPECT_NEAR(testStoreValues[2], 2.0f, 0.002f);
            case 2:
                EXPECT_NEAR(testStoreValues[1], 0.0f, 0.002f);
            case 1:
                EXPECT_NEAR(testStoreValues[0], -1.0f, 0.002f);
                break;
            }
        }
    }

    template <typename VectorType>
    void TestCast()
    {
        int32_t testLoadValues[4] = { -1, 0, 2, 4 };
        float testStoreFloatValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        int32_t testStoreInt32Values[4] = { 0, 0, 0, 0 };

        typename VectorType::Int32Type sourceVector = VectorType::LoadUnaligned(testLoadValues);

        typename VectorType::FloatType testVector1 = VectorType::CastToFloat(sourceVector);
        VectorType::StoreUnaligned(testStoreFloatValues, testVector1);

        EXPECT_TRUE(AZStd::isnan(testStoreFloatValues[0]));
        if constexpr (VectorType::ElementCount > 1)
        {
            EXPECT_NEAR(testStoreFloatValues[1], 0.0f, 0.002f);
        }

        typename VectorType::Int32Type testVector2 = VectorType::CastToInt(testVector1);
        VectorType::StoreUnaligned(testStoreInt32Values, testVector2);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreInt32Values[i], ::testing::Eq(testLoadValues[i]));
        }
    }

    template <typename VectorType>
    void TestZeroVectorFloat()
    {
        const typename Simd::Vec4::FloatType testVector = Simd::Vec4::ZeroFloat();

        float testStoreValues[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        Simd::Vec4::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::FloatNear(0.0f, AZ::Constants::Tolerance));
        }
    }

    template <typename VectorType>
    void TestZeroVectorInt()
    {
        const typename Simd::Vec4::Int32Type testVector = Simd::Vec4::ZeroInt();

        int32_t testStoreValues[4] = { 1, 1, 1, 1 };

        Simd::Vec4::StoreUnaligned(testStoreValues, testVector);

        for (int32_t i = 0; i < VectorType::ElementCount; ++i)
        {
            EXPECT_THAT(testStoreValues[i], ::testing::Eq(0));
        }
    }

    TEST(MATH_SimdMath, TestToVec1InVec2)
    {
        TestToVec1<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestFromVec1InVec2)
    {
        TestFromVec1<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestToVec1InVec3)
    {
        TestToVec1<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestFromVec1InVec3)
    {
        TestFromVec1<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestToVec2InVec3)
    {
        TestToVec2<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestFromVec2InVec3)
    {
        TestFromVec2<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestToVec1InVec4)
    {
        TestToVec1<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestFromVec1InVec4)
    {
        TestFromVec1<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestToVec2InVec4)
    {
        TestToVec2<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestFromVec2InVec4)
    {
        TestFromVec2<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestToVec3InVec4)
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float testStoreValues[4] = { -1.0f, -1.0f, -1.0f, -1.0f };

        typename Simd::Vec4::FloatType testVector = Simd::Vec4::LoadUnaligned(testLoadValues);
        Simd::Vec3::FloatType testVec3 = Simd::Vec4::ToVec3(testVector);
        Simd::Vec3::StoreUnaligned(testStoreValues, testVec3);

        for (int32_t i = 0; i < Simd::Vec3::ElementCount; ++i)
        {
            EXPECT_NEAR(testLoadValues[i], testStoreValues[i], AZ::Constants::Tolerance);
        }
    }

    TEST(MATH_SimdMath, TestFromVec3InVec4)
    {
        float testLoadValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float testStoreValues[4] = { -1.0f, -1.0f, -1.0f, -1.0f };

        Simd::Vec3::FloatType testVec3 = Simd::Vec3::LoadUnaligned(testLoadValues);
        typename Simd::Vec4::FloatType testVector = Simd::Vec4::FromVec3(testVec3);
        Simd::Vec4::StoreUnaligned(testStoreValues, testVector);

        // The first 3 elements must be set to (Vec3.x, Vec3.y, Vec3.z)
        for (int32_t i = 0; i < Simd::Vec3::ElementCount; ++i)
        {
            EXPECT_NEAR(testLoadValues[i], testStoreValues[i], AZ::Constants::Tolerance);
        }
        // The rest of elements must be set to zero
        for (int32_t i = Simd::Vec3::ElementCount; i < Simd::Vec4::ElementCount; ++i)
        {
            EXPECT_NEAR(0.0f, testStoreValues[i], AZ::Constants::Tolerance);
        }
    }

    TEST(MATH_SimdMath, TestLoadStoreFloatVec1)
    {
        TestLoadStoreFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestLoadStoreFloatVec2)
    {
        TestLoadStoreFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestLoadStoreFloatVec3)
    {
        TestLoadStoreFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestLoadStoreFloatVec4)
    {
        TestLoadStoreFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestLoadStoreIntVec1)
    {
        TestLoadStoreInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestLoadStoreIntVec2)
    {
        TestLoadStoreInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestLoadStoreIntVec3)
    {
        TestLoadStoreInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestLoadStoreIntVec4)
    {
        TestLoadStoreInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSelectFirstVec1)
    {
        TestSelectFirst<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestSelectFirstVec2)
    {
        TestSelectFirst<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestSelectSecondVec2)
    {
        TestSelectSecond<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestSelectFirstVec3)
    {
        TestSelectFirst<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestSelectSecondVec3)
    {
        TestSelectSecond<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestSelectThirdVec3)
    {
        TestSelectThird<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestSelectFirstVec4)
    {
        TestSelectFirst<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSelectSecondVec4)
    {
        TestSelectSecond<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSelectThirdVec4)
    {
        TestSelectThird<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSelectFourthVec4)
    {
        TestSelectFourth<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSplatFloatVec1)
    {
        TestSplatFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestSplatFloatVec2)
    {
        TestSplatFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestSplatFloatVec3)
    {
        TestSplatFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestSplatFloatVec4)
    {
        TestSplatFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSplatIntVec1)
    {
        TestSplatInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestSplatIntVec2)
    {
        TestSplatInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestSplatIntVec3)
    {
        TestSplatInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestSplatIntVec4)
    {
        TestSplatInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSplatFirstVec2)
    {
        TestSplatFirst<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestSplatSecondVec2)
    {
        TestSplatSecond<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestSplatFirstVec3)
    {
        TestSplatFirst<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestSplatSecondVec3)
    {
        TestSplatSecond<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestSplatThirdVec3)
    {
        TestSplatThird<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestSplatFirstVec4)
    {
        TestSplatFirst<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSplatSecondVec4)
    {
        TestSplatSecond<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSplatThirdVec4)
    {
        TestSplatThird<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSplatFourthVec4)
    {
        TestSplatFourth<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestReplaceFirstVec2)
    {
        TestReplaceFirst<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestReplaceSecondVec2)
    {
        TestReplaceSecond<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestReplaceFirstVec3)
    {
        TestReplaceFirst<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestReplaceSecondVec3)
    {
        TestReplaceSecond<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestReplaceThirdVec3)
    {
        TestReplaceThird<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestReplaceFirstVec4)
    {
        TestReplaceFirst<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestReplaceSecondVec4)
    {
        TestReplaceSecond<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestReplaceThirdVec4)
    {
        TestReplaceThird<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestReplaceFourthVec4)
    {
        TestReplaceFourth<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestLoadImmediateFloatVec1)
    {
        TestLoadImmediateFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestLoadImmediateFloatVec2)
    {
        TestLoadImmediateFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestLoadImmediateFloatVec3)
    {
        TestLoadImmediateFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestLoadImmediateFloatVec4)
    {
        TestLoadImmediateFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestLoadImmediateIntVec1)
    {
        TestLoadImmediateInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestLoadImmediateIntVec2)
    {
        TestLoadImmediateInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestLoadImmediateIntVec3)
    {
        TestLoadImmediateInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestLoadImmediateIntVec4)
    {
        TestLoadImmediateInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestAddSubMulDivFloatVec1)
    {
        TestAddSubMulDivFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestAddSubMulDivFloatVec2)
    {
        TestAddSubMulDivFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestAddSubMulDivFloatVec3)
    {
        TestAddSubMulDivFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestAddSubMulDivFloatVec4)
    {
        TestAddSubMulDivFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestAddSubMulIntVec1)
    {
        TestAddSubMulInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestAddSubMulIntVec2)
    {
        TestAddSubMulInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestAddSubMulIntVec3)
    {
        TestAddSubMulInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestAddSubMulIntVec4)
    {
        TestAddSubMulInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestNotFloatVec1)
    {
        TestNotFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestNotFloatVec2)
    {
        TestNotFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestNotFloatVec3)
    {
        TestNotFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestNotFloatVec4)
    {
        TestNotFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestAndAndnotOrFloatVec1)
    {
        TestAndAndnotOrFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestAndAndnotOrFloatVec2)
    {
        TestAndAndnotOrFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestAndAndnotOrFloatVec3)
    {
        TestAndAndnotOrFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestAndAndnotOrFloatVec4)
    {
        TestAndAndnotOrFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestXorFloatVec1)
    {
        TestXorFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestXorFloatVec2)
    {
        TestXorFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestXorFloatVec3)
    {
        TestXorFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestXorFloatVec4)
    {
        TestXorFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestNotIntVec1)
    {
        TestNotInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestNotIntVec2)
    {
        TestNotInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestNotIntVec3)
    {
        TestNotInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestNotIntVec4)
    {
        TestNotInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestAndAndnotOrIntVec1)
    {
        TestAndAndnotOrInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestAndAndnotOrIntVec2)
    {
        TestAndAndnotOrInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestAndAndnotOrIntVec3)
    {
        TestAndAndnotOrInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestAndAndnotOrIntVec4)
    {
        TestAndAndnotOrInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestXorIntVec1)
    {
        TestXorInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestXorIntVec2)
    {
        TestXorInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestXorIntVec3)
    {
        TestXorInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestXorIntVec4)
    {
        TestXorInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestMinMaxIntVec1)
    {
        TestMinMaxInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestMinMaxIntVec2)
    {
        TestMinMaxInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestMinMaxIntVec3)
    {
        TestMinMaxInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestMinMaxIntVec4)
    {
        TestMinMaxInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestClampIntVec1)
    {
        TestClampInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestClampIntVec2)
    {
        TestClampInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestClampIntVec3)
    {
        TestClampInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestClampIntVec4)
    {
        TestClampInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestFloorFloatVec1)
    {
        TestFloorFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestFloorFloatVec2)
    {
        TestFloorFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestFloorFloatVec3)
    {
        TestFloorFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestFloorFloatVec4)
    {
        TestFloorFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestCeilFloatVec1)
    {
        TestCeilFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestCeilFloatVec2)
    {
        TestCeilFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestCeilFloatVec3)
    {
        TestCeilFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestCeilFloatVec4)
    {
        TestCeilFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestRoundFloatVec1)
    {
        TestRoundFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestRoundFloatVec2)
    {
        TestRoundFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestRoundFloatVec3)
    {
        TestRoundFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestRoundFloatVec4)
    {
        TestRoundFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestTruncateFloatVec1)
    {
        TestTruncateFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestTruncateFloatVec2)
    {
        TestTruncateFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestTruncateFloatVec3)
    {
        TestTruncateFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestTruncateFloatVec4)
    {
        TestTruncateFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestMinMaxFloatVec1)
    {
        TestMinMaxFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestMinMaxFloatVec2)
    {
        TestMinMaxFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestMinMaxFloatVec3)
    {
        TestMinMaxFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestMinMaxFloatVec4)
    {
        TestMinMaxFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestClampFloatVec1)
    {
        TestClampFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestClampFloatVec2)
    {
        TestClampFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestClampFloatVec3)
    {
        TestClampFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestClampFloatVec4)
    {
        TestClampFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestCompareFloatVec1)
    {
        TestCompareFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestCompareFloatVec2)
    {
        TestCompareFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestCompareFloatVec3)
    {
        TestCompareFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestCompareFloatVec4)
    {
        TestCompareFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestCompareAllFloatVec1)
    {
        TestCompareAllFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestCompareAllFloatVec2)
    {
        TestCompareAllFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestCompareAllFloatVec3)
    {
        TestCompareAllFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestCompareAllFloatVec4)
    {
        TestCompareAllFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestCompareIntVec1)
    {
        TestCompareInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestCompareIntVec2)
    {
        TestCompareInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestCompareIntVec3)
    {
        TestCompareInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestCompareIntVec4)
    {
        TestCompareInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestCompareAllIntVec1)
    {
        TestCompareAllInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestCompareAllIntVec2)
    {
        TestCompareAllInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestCompareAllIntVec3)
    {
        TestCompareAllInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestCompareAllIntVec4)
    {
        TestCompareAllInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestReciprocalFloatVec1)
    {
        TestReciprocalFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestReciprocalFloatVec2)
    {
        TestReciprocalFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestReciprocalFloatVec3)
    {
        TestReciprocalFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestReciprocalFloatVec4)
    {
        TestReciprocalFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSqrtInvSqrtFloatVec1)
    {
        TestSqrtInvSqrtFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestSqrtInvSqrtFloatVec2)
    {
        TestSqrtInvSqrtFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestSqrtInvSqrtFloatVec3)
    {
        TestSqrtInvSqrtFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestSqrtInvSqrtFloatVec4)
    {
        TestSqrtInvSqrtFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestSinVec1)
    {
        TestSin<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestSinVec2)
    {
        TestSin<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestSinVec3)
    {
        TestSin<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestSinVec4)
    {
        TestSin<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestCosVec1)
    {
        TestCos<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestCosVec2)
    {
        TestCos<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestCosVec3)
    {
        TestCos<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestCosVec4)
    {
        TestCos<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestAcosVec1)
    {
        TestAcos<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestAcosVec2)
    {
        TestAcos<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestAcosVec3)
    {
        TestAcos<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestAcosVec4)
    {
        TestAcos<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestAtanVec1)
    {
        TestAtan<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestAtanVec2)
    {
        TestAtan<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestAtanVec3)
    {
        TestAtan<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestAtanVec4)
    {
        TestAtan<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestAtan2Vec1)
    {
        TestAtan2<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestAtan2Vec2)
    {
        TestAtan2<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestAtan2Vec3)
    {
        TestAtan2<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestAtan2Vec4)
    {
        TestAtan2<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestExpEstimateVec1)
    {
        TestExpEstimate<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestExpEstimateVec2)
    {
        TestExpEstimate<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestExpEstimateVec3)
    {
        TestExpEstimate<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestExpEstimateVec4)
    {
        TestExpEstimate<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestDotFloatVec2)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec2::FloatType sourceVector1 = Simd::Vec2::LoadImmediate(-1.0f, 16.0f);
        Simd::Vec2::FloatType sourceVector2 = Simd::Vec2::LoadImmediate(-1.0f, 16.0f);

        Simd::Vec1::FloatType testDot = Simd::Vec2::Dot(sourceVector1, sourceVector2);
        Simd::Vec1::StoreUnaligned(testStoreValues, testDot);

        EXPECT_NEAR(testStoreValues[0], 257.0f, 0.002f);
    }

    TEST(MATH_SimdMath, TestDotFloatVec3)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec3::FloatType sourceVector1 = Simd::Vec3::LoadImmediate(-1.0f, 16.0f, 1.0f);
        Simd::Vec3::FloatType sourceVector2 = Simd::Vec3::LoadImmediate(-1.0f, 16.0f, 1.0f);

        Simd::Vec1::FloatType testDot = Simd::Vec3::Dot(sourceVector1, sourceVector2);
        Simd::Vec1::StoreUnaligned(testStoreValues, testDot);

        EXPECT_NEAR(testStoreValues[0], 258.0f, 0.002f);
    }

    TEST(MATH_SimdMath, TestDotFloatVec4)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec4::FloatType sourceVector1 = Simd::Vec4::LoadImmediate(-1.0f, 16.0f, 1.0f,  1.0f);
        Simd::Vec4::FloatType sourceVector2 = Simd::Vec4::LoadImmediate(-1.0f, 16.0f, 1.0f, -1.0f);

        Simd::Vec1::FloatType testDot = Simd::Vec4::Dot(sourceVector1, sourceVector2);
        Simd::Vec1::StoreUnaligned(testStoreValues, testDot);

        EXPECT_NEAR(testStoreValues[0], 257.0f, 0.002f);
    }

    TEST(MATH_SimdMath, TestCrossFloatVec3)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec3::FloatType sourceVector1 = Simd::Vec3::LoadImmediate(1.0f, 0.0f, 0.0f);
        Simd::Vec3::FloatType sourceVector2 = Simd::Vec3::LoadImmediate(0.0f, 1.0f, 0.0f);
        Simd::Vec3::FloatType sourceVector3 = Simd::Vec3::LoadImmediate(0.0f, 0.0f, 1.0f);

        {
            Simd::Vec3::FloatType testVector = Simd::Vec3::Cross(sourceVector1, sourceVector2);
            Simd::Vec3::StoreUnaligned(testStoreValues, testVector);

            EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[1], 0.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[2], 1.0f, 0.002f);
            // The 4th element value cannot be guaranteed, it generally sets garbage.
        }

        {
            Simd::Vec3::FloatType testVector = Simd::Vec3::Cross(sourceVector2, sourceVector3);
            Simd::Vec3::StoreUnaligned(testStoreValues, testVector);

            EXPECT_NEAR(testStoreValues[0], 1.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[1], 0.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[2], 0.0f, 0.002f);
            // The 4th element value cannot be guaranteed, it generally sets garbage.
        }

        {
            Simd::Vec3::FloatType testVector = Simd::Vec3::Cross(sourceVector3, sourceVector1);
            Simd::Vec3::StoreUnaligned(testStoreValues, testVector);

            EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[1], 1.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[2], 0.0f, 0.002f);
            // The 4th element value cannot be guaranteed, it generally sets garbage.
        }
    }

    TEST(MATH_SimdMath, TestMat3x3Inverse)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec3::FloatType rows[3];
        rows[0] = Simd::Vec3::LoadImmediate(2.0f, 0.0f, 0.0f);
        rows[1] = Simd::Vec3::LoadImmediate(0.0f, 2.0f, 0.0f);
        rows[2] = Simd::Vec3::LoadImmediate(0.0f, 0.0f, 2.0f);

        Simd::Vec3::Mat3x3Inverse(rows, rows);

        Simd::Vec3::StoreUnaligned(testStoreValues, rows[0]);
        EXPECT_NEAR(testStoreValues[0], 0.5f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 0.0f, 0.002f);

        Simd::Vec3::StoreUnaligned(testStoreValues, rows[1]);
        EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 0.5f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 0.0f, 0.002f);

        Simd::Vec3::StoreUnaligned(testStoreValues, rows[2]);
        EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 0.5f, 0.002f);
    }

    TEST(MATH_SimdMath, TestMat3x3Adjugate)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec3::FloatType rows[3];
        rows[0] = Simd::Vec3::LoadImmediate( 1.0f,  2.0f,  3.0f);
        rows[1] = Simd::Vec3::LoadImmediate( 2.0f,  0.0f,  1.0f);
        rows[2] = Simd::Vec3::LoadImmediate(-1.0f,  1.0f,  2.0f);

        Simd::Vec3::Mat3x3Adjugate(rows, rows);

        Simd::Vec3::StoreUnaligned(testStoreValues, rows[0]);
        EXPECT_NEAR(testStoreValues[0], -1.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], -1.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2],  2.0f, 0.002f);

        Simd::Vec3::StoreUnaligned(testStoreValues, rows[1]);
        EXPECT_NEAR(testStoreValues[0], -5.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1],  5.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2],  5.0f, 0.002f);

        Simd::Vec3::StoreUnaligned(testStoreValues, rows[2]);
        EXPECT_NEAR(testStoreValues[0],  2.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], -3.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], -4.0f, 0.002f);
    }

    TEST(MATH_SimdMath, TestMat3x3Transpose)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec3::FloatType rows[3];
        rows[0] = Simd::Vec3::LoadImmediate(1.0f, 2.0f, 3.0f);
        rows[1] = Simd::Vec3::LoadImmediate(4.0f, 5.0f, 6.0f);
        rows[2] = Simd::Vec3::LoadImmediate(7.0f, 8.0f, 9.0f);

        Simd::Vec3::Mat3x3Transpose(rows, rows);

        Simd::Vec3::StoreUnaligned(testStoreValues, rows[0]);
        EXPECT_NEAR(testStoreValues[0], 1.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 4.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 7.0f, 0.002f);

        Simd::Vec3::StoreUnaligned(testStoreValues, rows[1]);
        EXPECT_NEAR(testStoreValues[0], 2.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 5.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 8.0f, 0.002f);

        Simd::Vec3::StoreUnaligned(testStoreValues, rows[2]);
        EXPECT_NEAR(testStoreValues[0], 3.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 6.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 9.0f, 0.002f);
    }

    TEST(MATH_SimdMath, TestMat3x3Multiply)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec3::FloatType rowsA[3];
        rowsA[0] = Simd::Vec3::LoadImmediate(2.0f, 0.0f, 0.0f);
        rowsA[1] = Simd::Vec3::LoadImmediate(0.0f, 2.0f, 0.0f);
        rowsA[2] = Simd::Vec3::LoadImmediate(0.0f, 0.0f, 2.0f);

        Simd::Vec3::FloatType rowsB[3];
        rowsB[0] = Simd::Vec3::LoadImmediate(0.5f, 0.0f, 0.0f);
        rowsB[1] = Simd::Vec3::LoadImmediate(0.0f, 0.5f, 0.0f);
        rowsB[2] = Simd::Vec3::LoadImmediate(0.0f, 0.0f, 0.5f);

        Simd::Vec3::FloatType out[3];
        Simd::Vec3::Mat3x3Multiply(rowsA, rowsB, out);

        Simd::Vec3::StoreUnaligned(testStoreValues, out[0]);
        EXPECT_NEAR(testStoreValues[0], 1.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 0.0f, 0.002f);

        Simd::Vec3::StoreUnaligned(testStoreValues, out[1]);
        EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 1.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 0.0f, 0.002f);

        Simd::Vec3::StoreUnaligned(testStoreValues, out[2]);
        EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 1.0f, 0.002f);
    }

    TEST(MATH_SimdMath, TestMat3x3TransposeMultiply)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec3::FloatType rowsA[3];
        rowsA[0] = Simd::Vec3::LoadImmediate(2.0f, 0.0f, 0.0f);
        rowsA[1] = Simd::Vec3::LoadImmediate(0.0f, 2.0f, 0.0f);
        rowsA[2] = Simd::Vec3::LoadImmediate(0.0f, 0.0f, 2.0f);

        Simd::Vec3::FloatType rowsB[3];
        rowsB[0] = Simd::Vec3::LoadImmediate(0.5f, 0.0f, 0.0f);
        rowsB[1] = Simd::Vec3::LoadImmediate(0.0f, 0.5f, 0.0f);
        rowsB[2] = Simd::Vec3::LoadImmediate(0.0f, 0.0f, 0.5f);

        Simd::Vec3::FloatType out[3];
        Simd::Vec3::Mat3x3TransposeMultiply(rowsA, rowsB, out);

        Simd::Vec3::StoreUnaligned(testStoreValues, out[0]);
        EXPECT_NEAR(testStoreValues[0], 1.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 0.0f, 0.002f);

        Simd::Vec3::StoreUnaligned(testStoreValues, out[1]);
        EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 1.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 0.0f, 0.002f);

        Simd::Vec3::StoreUnaligned(testStoreValues, out[2]);
        EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 1.0f, 0.002f);
    }

    TEST(MATH_SimdMath, TestQuaternion)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        AZ::Quaternion quat1 = AZ::Quaternion::CreateRotationZ(DegToRad(90.0f));
        Simd::Vec4::FloatType quatVec1 = Simd::Vec4::LoadImmediate(quat1.GetX(), quat1.GetY(), quat1.GetZ(), quat1.GetW());

        AZ::Quaternion quat2 = AZ::Quaternion::CreateRotationX(DegToRad(90.0f));
        Simd::Vec4::FloatType quatVec2 = Simd::Vec4::LoadImmediate(quat2.GetX(), quat2.GetY(), quat2.GetZ(), quat2.GetW());

        Simd::Vec3::FloatType yAxisVec = Simd::Vec3::LoadImmediate(0.0f, 1.0f, 0.0f);

        {
            Simd::Vec4::FloatType testVector = Simd::Vec4::QuaternionMultiply(quatVec1, quatVec2);
            Simd::Vec4::StoreUnaligned(testStoreValues, testVector);

            EXPECT_NEAR(testStoreValues[0], 0.5f, 0.003f); // TODO: Get new trig, improve precision
            EXPECT_NEAR(testStoreValues[1], 0.5f, 0.003f); // TODO: Get new trig, improve precision
            EXPECT_NEAR(testStoreValues[2], 0.5f, 0.003f); // TODO: Get new trig, improve precision
            EXPECT_NEAR(testStoreValues[3], 0.5f, 0.003f); // TODO: Get new trig, improve precision
        }

        {
            Simd::Vec3::FloatType testVector = Simd::Vec4::QuaternionTransform(quatVec1, yAxisVec);
            Simd::Vec3::StoreUnaligned(testStoreValues, testVector);

            EXPECT_NEAR(testStoreValues[0], -1.0f, 0.003f); // TODO: Get new trig, improve precision
            EXPECT_NEAR(testStoreValues[1],  0.0f, 0.003f); // TODO: Get new trig, improve precision
            EXPECT_NEAR(testStoreValues[2],  0.0f, 0.003f); // TODO: Get new trig, improve precision
        }

        {
            Simd::Vec3::FloatType testVector = Simd::Vec4::QuaternionTransform(quatVec2, yAxisVec);
            Simd::Vec3::StoreUnaligned(testStoreValues, testVector);

            EXPECT_NEAR(testStoreValues[0], 0.0f, 0.003f); // TODO: Get new trig, improve precision
            EXPECT_NEAR(testStoreValues[1], 0.0f, 0.003f); // TODO: Get new trig, improve precision
            EXPECT_NEAR(testStoreValues[2], 1.0f, 0.003f); // TODO: Get new trig, improve precision
        }
    }

    TEST(MATH_SimdMath, TestPlane)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        // Point offset orthogonal to the plane normal, offset should be discarded
        {
            Simd::Vec3::FloatType normalVec = Simd::Vec3::LoadImmediate(0.0f, 1.0f, 0.0f);
            Simd::Vec3::FloatType pointVec  = Simd::Vec3::LoadImmediate(1.0f, 0.0f, 0.0f);

            Simd::Vec4::FloatType planeVec = Simd::Vec4::ConstructPlane(normalVec, pointVec);
            Simd::Vec4::StoreUnaligned(testStoreValues, planeVec);

            EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[1], 1.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[2], 0.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[3], 0.0f, 0.002f);

            Simd::Vec3::FloatType testPoint = Simd::Vec3::LoadImmediate(0.0f, 10.0f, 0.0f);
            Simd::Vec1::FloatType testDist = Simd::Vec4::PlaneDistance(planeVec, testPoint);
            Simd::Vec1::StoreUnaligned(testStoreValues, testDist);

            EXPECT_NEAR(testStoreValues[0], 10.0f, 0.002f);
        }

        // Point offset along the plane normal, offset should be preserved in distance calculations
        {
            Simd::Vec3::FloatType normalVec = Simd::Vec3::LoadImmediate(0.0f, 1.0f, 0.0f);
            Simd::Vec3::FloatType pointVec = Simd::Vec3::LoadImmediate(0.0f, 10.0f, 0.0f);

            Simd::Vec4::FloatType planeVec = Simd::Vec4::ConstructPlane(normalVec, pointVec);
            Simd::Vec4::StoreUnaligned(testStoreValues, planeVec);

            EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[1], 1.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[2], 0.0f, 0.002f);
            EXPECT_NEAR(testStoreValues[3], -10.0f, 0.002f);

            Simd::Vec3::FloatType testPoint = Simd::Vec3::LoadImmediate(0.0f, -5.0f, 0.0f);
            Simd::Vec1::FloatType testDist = Simd::Vec4::PlaneDistance(planeVec, testPoint);
            Simd::Vec1::StoreUnaligned(testStoreValues, testDist);

            EXPECT_NEAR(testStoreValues[0], -15.0f, 0.002f);
        }
    }

    TEST(MATH_SimdMath, TestMat3x4Transpose)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec4::FloatType rows[4];
        rows[0] = Simd::Vec4::LoadImmediate(0.0f, 1.0f, 2.0f, 3.0f);
        rows[1] = Simd::Vec4::LoadImmediate(4.0f, 5.0f, 6.0f, 7.0f);
        rows[2] = Simd::Vec4::LoadImmediate(8.0f, 9.0f, -1.0f, -2.0f);

        Simd::Vec4::Mat3x4Transpose(rows, rows);

        Simd::Vec4::StoreUnaligned(testStoreValues, rows[0]);
        EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 4.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 8.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[3], 0.0f, 0.002f);

        Simd::Vec4::StoreUnaligned(testStoreValues, rows[1]);
        EXPECT_NEAR(testStoreValues[0], 1.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 5.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 9.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[3], 0.0f, 0.002f);

        Simd::Vec4::StoreUnaligned(testStoreValues, rows[2]);
        EXPECT_NEAR(testStoreValues[0], 2.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 6.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], -1.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[3], 0.0f, 0.002f);
    }

    TEST(MATH_SimdMath, TestMat4x4Transpose)
    {
        float testStoreValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        Simd::Vec4::FloatType rows[4];
        rows[0] = Simd::Vec4::LoadImmediate(0.0f, 1.0f, 2.0f, 3.0f);
        rows[1] = Simd::Vec4::LoadImmediate(4.0f, 5.0f, 6.0f, 7.0f);
        rows[2] = Simd::Vec4::LoadImmediate(8.0f, 9.0f, 9.1f, 9.9f);
        rows[3] = Simd::Vec4::LoadImmediate(-1.0f, -2.0f, -3.0f, -4.0f);

        Simd::Vec4::Mat4x4Transpose(rows, rows);

        Simd::Vec4::StoreUnaligned(testStoreValues, rows[0]);
        EXPECT_NEAR(testStoreValues[0], 0.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 4.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 8.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[3], -1.0f, 0.002f);

        Simd::Vec4::StoreUnaligned(testStoreValues, rows[1]);
        EXPECT_NEAR(testStoreValues[0], 1.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 5.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 9.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[3], -2.0f, 0.002f);

        Simd::Vec4::StoreUnaligned(testStoreValues, rows[2]);
        EXPECT_NEAR(testStoreValues[0], 2.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 6.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 9.1f, 0.002f);
        EXPECT_NEAR(testStoreValues[3], -3.0f, 0.002f);

        Simd::Vec4::StoreUnaligned(testStoreValues, rows[3]);
        EXPECT_NEAR(testStoreValues[0], 3.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[1], 7.0f, 0.002f);
        EXPECT_NEAR(testStoreValues[2], 9.9f, 0.002f);
        EXPECT_NEAR(testStoreValues[3], -4.0f, 0.002f);
    }

    TEST(MATH_SimdMath, TestConvertToIntVec1)
    {
        TestConvertToInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestConvertToIntVec2)
    {
        TestConvertToInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestConvertToIntVec3)
    {
        TestConvertToInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestConvertToIntVec4)
    {
        TestConvertToInt<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestConvertToIntNearestVec1)
    {
        TestConvertToIntNearest<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestConvertToIntNearestVec2)
    {
        TestConvertToIntNearest<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestConvertToIntNearestVec3)
    {
        TestConvertToIntNearest<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestConvertToIntNearestVec4)
    {
        TestConvertToIntNearest<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestConvertToFloatVec1)
    {
        TestConvertToFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestConvertToFloatVec2)
    {
        TestConvertToFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestConvertToFloatVec3)
    {
        TestConvertToFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestConvertToFloatVec4)
    {
        TestConvertToFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestCastVec1)
    {
        TestCast<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestCastVec2)
    {
        TestCast<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestCastVec3)
    {
        TestCast<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestCastVec4)
    {
        TestCast<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestZeroVectorFloatVec1)
    {
        TestZeroVectorFloat<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestZeroVectorFloatVec2)
    {
        TestZeroVectorFloat<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestZeroVectorFloatVec3)
    {
        TestZeroVectorFloat<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestZeroVectorFloatVec4)
    {
        TestZeroVectorFloat<Simd::Vec4>();
    }

    TEST(MATH_SimdMath, TestZeroVectorIntVec1)
    {
        TestZeroVectorInt<Simd::Vec1>();
    }

    TEST(MATH_SimdMath, TestZeroVectorIntVec2)
    {
        TestZeroVectorInt<Simd::Vec2>();
    }

    TEST(MATH_SimdMath, TestZeroVectorIntVec3)
    {
        TestZeroVectorInt<Simd::Vec3>();
    }

    TEST(MATH_SimdMath, TestZeroVectorIntVec4)
    {
        TestZeroVectorInt<Simd::Vec4>();
    }
}
