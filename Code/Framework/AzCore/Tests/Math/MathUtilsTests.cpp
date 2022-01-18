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
        const float epsilonF = std::numeric_limits<float>::epsilon();
        const float doesntMatterF = std::numeric_limits<float>::signaling_NaN();
        float lowerF = 2.3f, upperF = 2.3f;
        EXPECT_EQ(0.0f, AZ::LerpInverse(lowerF, upperF, doesntMatterF));
        EXPECT_EQ(0.0f, AZ::LerpInverse(0.0f, 0.5f * epsilonF, doesntMatterF));
        EXPECT_EQ(0.0f, AZ::LerpInverse(0.0f, 5.0f * epsilonF, 0.0f));
        EXPECT_NEAR(0.4f, AZ::LerpInverse(0.0f, 5.0f * epsilonF, 2.0f * epsilonF), epsilonF);
        EXPECT_NEAR(0.6f, AZ::LerpInverse(1.0f, 1.0f + 5.0f * epsilonF, 1.0f + 3.0f * epsilonF), epsilonF);
        EXPECT_NEAR(1.0f, AZ::LerpInverse(1.0f, 1.0f + 5.0f * epsilonF, 1.0f + 5.0f * epsilonF), epsilonF);

        // Double
        const double epsilonD = std::numeric_limits<double>::epsilon();
        const double doesntMatterD = std::numeric_limits<double>::signaling_NaN();
        double lowerD = 2.3, upperD = 2.3;
        EXPECT_EQ(0.0, AZ::LerpInverse(lowerD, upperD, doesntMatterD));
        EXPECT_EQ(0.0, AZ::LerpInverse(0.0, 0.5 * epsilonD, doesntMatterD));
        EXPECT_EQ(0.0, AZ::LerpInverse(0.0, 5.0 * epsilonD, 0.0));
        EXPECT_NEAR(0.4, AZ::LerpInverse(0.0, 5.0 * epsilonD, 2.0 * epsilonD), epsilonD);
        EXPECT_NEAR(0.6, AZ::LerpInverse(1.0, 1.0 + 5.0 * epsilonD, 1.0 + 3.0 * epsilonD), epsilonD);
        EXPECT_NEAR(1.0, AZ::LerpInverse(1.0, 1.0 + 5.0 * epsilonD, 1.0 + 5.0 * epsilonD), epsilonD);
    }

    class RoundUpToMultipleTestsFixture
        : public ScopedAllocatorSetupFixture
        , public ::testing::WithParamInterface<uint32_t>
    {
    };

    template <typename T>
    void TestRoundUpToMultipleIsCorrect(T alignment)
    {
        // Example: alignment: 4
        // inputValue:     0 1 2 3 4 5 6 7 8 ...
        // expectedOutput: 0 4 4 4 4 8 8 8 8 ...
        AZStd::vector<T> expectedOutput;
        constexpr T iterations = 4;
        expectedOutput.reserve(iterations * alignment + 1);
        expectedOutput.push_back(0);

        for (T i = 1; i <= iterations; ++i)
        {
            for (T j = 0; j < alignment; ++j)
            {
                expectedOutput.push_back(i * alignment);
            }
        }

        for (T inputValue = 0; inputValue < expectedOutput.size(); ++inputValue)
        {
            T result = RoundUpToMultiple(inputValue, alignment);
            EXPECT_EQ(result, expectedOutput[inputValue]);
        }
    }

    TEST_P(RoundUpToMultipleTestsFixture, RoundUpToMultipleUInt32_ValidInput_IsCorrect)
    {
        uint32_t alignment = GetParam();
        TestRoundUpToMultipleIsCorrect(alignment);
    }

    TEST_P(RoundUpToMultipleTestsFixture, RoundUpToMultipleUInt64_ValidInput_IsCorrect)
    {
        uint64_t alignment = GetParam();
        TestRoundUpToMultipleIsCorrect(alignment);
    }
    
    INSTANTIATE_TEST_CASE_P(
        MATH_RoundUpToMultiple,
        RoundUpToMultipleTestsFixture,
        // Multiples that we're going to test rounding up to
        // Test with some low numbers, prime numbers, power of two numbers, and non-prime non-power-of-two numbers
        ::testing::ValuesIn({1u,2u,3u,4u,5u,8u,9u,12u,13u}));
}
