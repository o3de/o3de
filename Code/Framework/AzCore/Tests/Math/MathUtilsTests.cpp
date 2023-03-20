/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Lerp, Test)
    {
        // Float
        EXPECT_EQ(2.5f, AZ::Lerp(2.0f, 4.0f, 0.25f));
        EXPECT_EQ(6.0f, AZ::Lerp(2.0f, 4.0f, 2.0f));

        // Double
        EXPECT_EQ(3.5, AZ::Lerp(2.0, 4.0, 0.75));
        EXPECT_EQ(0.0, AZ::Lerp(2.0, 4.0, -1.0));
    }

    TEST(MATH_LerpInverse, Test)
    {
        // Float
        EXPECT_NEAR(0.25f, AZ::LerpInverse(2.0f, 4.0f, 2.5f), 0.0001f);
        EXPECT_NEAR(2.0f, AZ::LerpInverse(2.0f, 4.0f, 6.0f), 0.0001f);

        // Double
        EXPECT_NEAR(0.75, AZ::LerpInverse(2.0, 4.0, 3.5), 0.0001);
        EXPECT_NEAR(-1.0, AZ::LerpInverse(2.0, 4.0, 0.0), 0.0001);

        // min/max need to be substantially different to return a useful t value

        // Float
        const float epsilonF = AZStd::numeric_limits<float>::epsilon();
        const float doesntMatterF = AZStd::numeric_limits<float>::signaling_NaN();
        float lowerF = 2.3f, upperF = 2.3f;
        EXPECT_EQ(0.0f, AZ::LerpInverse(lowerF, upperF, doesntMatterF));
        EXPECT_EQ(0.0f, AZ::LerpInverse(0.0f, 0.5f * epsilonF, doesntMatterF));
        EXPECT_EQ(0.0f, AZ::LerpInverse(0.0f, 5.0f * epsilonF, 0.0f));
        EXPECT_NEAR(0.4f, AZ::LerpInverse(0.0f, 5.0f * epsilonF, 2.0f * epsilonF), epsilonF);
        EXPECT_NEAR(0.6f, AZ::LerpInverse(1.0f, 1.0f + 5.0f * epsilonF, 1.0f + 3.0f * epsilonF), epsilonF);
        EXPECT_NEAR(1.0f, AZ::LerpInverse(1.0f, 1.0f + 5.0f * epsilonF, 1.0f + 5.0f * epsilonF), epsilonF);

        // Double
        const double epsilonD = AZStd::numeric_limits<double>::epsilon();
        const double doesntMatterD = AZStd::numeric_limits<double>::signaling_NaN();
        double lowerD = 2.3, upperD = 2.3;
        EXPECT_EQ(0.0, AZ::LerpInverse(lowerD, upperD, doesntMatterD));
        EXPECT_EQ(0.0, AZ::LerpInverse(0.0, 0.5 * epsilonD, doesntMatterD));
        EXPECT_EQ(0.0, AZ::LerpInverse(0.0, 5.0 * epsilonD, 0.0));
        EXPECT_NEAR(0.4, AZ::LerpInverse(0.0, 5.0 * epsilonD, 2.0 * epsilonD), epsilonD);
        EXPECT_NEAR(0.6, AZ::LerpInverse(1.0, 1.0 + 5.0 * epsilonD, 1.0 + 3.0 * epsilonD), epsilonD);
        EXPECT_NEAR(1.0, AZ::LerpInverse(1.0, 1.0 + 5.0 * epsilonD, 1.0 + 5.0 * epsilonD), epsilonD);
    }

    template <typename T>
    void TestRoundUpToMultipleIsCorrect()
    {
        // Example: alignment: 4
        // inputValue:     0 1 2 3 4 5 6 7 8 ...
        // expectedOutput: 0 4 4 4 4 8 8 8 8 ...
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(0) , static_cast<T>(1)) , 0);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(1) , static_cast<T>(1)) , 1);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(2) , static_cast<T>(1)) , 2);

        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(0) , static_cast<T>(2)) , 0);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(1) , static_cast<T>(2)) , 2);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(2) , static_cast<T>(2)) , 2);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(3) , static_cast<T>(2)) , 4);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(4) , static_cast<T>(2)) , 4);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(5) , static_cast<T>(2)) , 6);

        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(0) , static_cast<T>(8)) , 0);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(1) , static_cast<T>(8)) , 8);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(7) , static_cast<T>(8)) , 8);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(8) , static_cast<T>(8)) , 8);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(9) , static_cast<T>(8)) , 16);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(15), static_cast<T>(8)) , 16);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(16), static_cast<T>(8)) , 16);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(17), static_cast<T>(8)) , 24);

        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(0) , static_cast<T>(13)), 0);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(1) , static_cast<T>(13)), 13);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(9) , static_cast<T>(13)), 13);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(12), static_cast<T>(13)), 13);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(13), static_cast<T>(13)), 13);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(14), static_cast<T>(13)), 26);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(25), static_cast<T>(13)), 26);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(26), static_cast<T>(13)), 26);
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(27), static_cast<T>(13)), 39);

        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(0), AZStd::numeric_limits<T>::max()), 0);

        T aVeryLargeNumberThatStillWontOverflow = AZStd::numeric_limits<T>::max() - 4;
        EXPECT_EQ(RoundUpToMultiple(static_cast<T>(1), aVeryLargeNumberThatStillWontOverflow), aVeryLargeNumberThatStillWontOverflow);
        EXPECT_EQ(RoundUpToMultiple(aVeryLargeNumberThatStillWontOverflow, static_cast<T>(1)), aVeryLargeNumberThatStillWontOverflow);
    }

    TEST(RoundUpToMultipleTest, RoundUpToMultipleUInt32_ValidInput_IsCorrect)
    {
        TestRoundUpToMultipleIsCorrect<uint32_t>();
    }

    TEST(RoundUpToMultipleTest, RoundUpToMultipleUInt64_ValidInput_IsCorrect)
    {
        TestRoundUpToMultipleIsCorrect<uint64_t>();
    }

    TEST(RoundUpToMultipleTest, RoundUpToMultipleSizeT_ValidInput_IsCorrect)
    {
        TestRoundUpToMultipleIsCorrect<size_t>();
    }

    template<typename T>
    void TestDivideAndRoundUpIsCorrect()
    {
        //! Example: alignment: 3
        //! Value:  0 1 2 3 4 5 6 7 8
        //! Result: 0 1 1 1 2 2 2 3 3
        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(0), static_cast<T>(3)), 0);
        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(1), static_cast<T>(3)), 1);
        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(2), static_cast<T>(3)), 1);
        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(3), static_cast<T>(3)), 1);
        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(4), static_cast<T>(3)), 2);
        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(5), static_cast<T>(3)), 2);
        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(6), static_cast<T>(3)), 2);
        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(7), static_cast<T>(3)), 3);
        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(8), static_cast<T>(3)), 3);

        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(0), AZStd::numeric_limits<T>::max()), 0);

        T aVeryLargeNumberThatStillWontOverflow = AZStd::numeric_limits<T>::max() - 4;
        EXPECT_EQ(DivideAndRoundUp(static_cast<T>(1), aVeryLargeNumberThatStillWontOverflow), static_cast<T>(1));
        EXPECT_EQ(DivideAndRoundUp(aVeryLargeNumberThatStillWontOverflow, static_cast<T>(1)), aVeryLargeNumberThatStillWontOverflow);

    }

    TEST(DivideAndRoundUpTest, DivideAndRoundUpUInt32_ValidInput_IsCorrect)
    {
        TestDivideAndRoundUpIsCorrect<uint32_t>();
    }

    TEST(DivideAndRoundUpTest, DivideAndRoundUpUInt64_ValidInput_IsCorrect)
    {
        TestDivideAndRoundUpIsCorrect<uint64_t>();
    }

    TEST(DivideAndRoundUpTest, DivideAndRoundUpSizeT_ValidInput_IsCorrect)
    {
        TestDivideAndRoundUpIsCorrect<size_t>();
    }

    // Test fixture for invalid inputs. We do not test for alignment=0
    // because that leads to undefined behavior with no consistent expected outcome 
    class RoundUpInvalidInputTestsFixture : public LeakDetectionFixture
    {
    };

    TEST_F(RoundUpInvalidInputTestsFixture, DividAndRoundUp_OverflowUint32_Assert)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        DivideAndRoundUp(
            static_cast<uint32_t>((AZStd::numeric_limits<uint32_t>::max() / 2) + 1),
            static_cast<uint32_t>((AZStd::numeric_limits<uint32_t>::max() / 2) + 1));
#ifdef AZ_DEBUG_BUILD
        // AZ_MATH_ASSERT only asserts during debug builds, so expect an assert here
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
#else
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
#endif
    }

    TEST_F(RoundUpInvalidInputTestsFixture, DividAndRoundUp_OverflowUint64_Assert)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        DivideAndRoundUp(
            static_cast<uint64_t>((AZStd::numeric_limits<uint64_t>::max() / 2) + 1),
            static_cast<uint64_t>((AZStd::numeric_limits<uint64_t>::max() / 2) + 1));
#ifdef AZ_DEBUG_BUILD
        // AZ_MATH_ASSERT only asserts during debug builds, so expect an assert here
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
#else
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
#endif
    }

    TEST_F(RoundUpInvalidInputTestsFixture, DividAndRoundUp_OverflowSizeT_Assert)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        DivideAndRoundUp(
            static_cast<size_t>((AZStd::numeric_limits<size_t>::max() / 2) + 1),
            static_cast<size_t>((AZStd::numeric_limits<size_t>::max() / 2) + 1));
#ifdef AZ_DEBUG_BUILD
        // AZ_MATH_ASSERT only asserts during debug builds, so expect an assert here
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
#else
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
#endif
    }
}
