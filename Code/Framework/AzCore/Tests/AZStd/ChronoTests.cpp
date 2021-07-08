/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <limits>

#include "UserTypes.h"

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // Fixtures

    // Fixture for non-typed tests
    class DurationTest
        : public AllocatorsFixture
    {
    protected:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
        }

        void TearDown() override
        {
            AllocatorsFixture::TearDown();
        }
    };

    /*
     * Helper Type Trait structure for adding expected value to typed
    */
    template<typename TestType, size_t RequiredBits, typename ExpectedPeriodType>
    struct DurationExpectation
    {
        using test_type = TestType;
        using rep = typename TestType::rep;
        using period = typename TestType::period;
        using expected_period = ExpectedPeriodType;
        static constexpr bool is_signed = AZStd::is_signed<int64_t>::value;
        static constexpr bool is_integral = AZStd::is_integral<int64_t>::value;
        static constexpr size_t required_bits = RequiredBits;
    };

    // Fixture for typed tests
    template<typename ExpectedResultTraits>
    class DurationTypedTest
        : public AllocatorsFixture
    {
    protected:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
        }

        void TearDown() override
        {
            AllocatorsFixture::TearDown();
        }
    };
    using ChronoTestTypes = ::testing::Types<
        DurationExpectation<AZStd::chrono::nanoseconds, 63, AZStd::nano>,
        DurationExpectation<AZStd::chrono::microseconds, 54, AZStd::micro>,
        DurationExpectation<AZStd::chrono::milliseconds, 44, AZStd::milli>,
        DurationExpectation<AZStd::chrono::seconds, 44, AZStd::ratio<1>>,
        DurationExpectation<AZStd::chrono::minutes, 28, AZStd::ratio<60>>,
        DurationExpectation<AZStd::chrono::hours, 22, AZStd::ratio<3600>>
    >;
    TYPED_TEST_CASE(DurationTypedTest, ChronoTestTypes);


    //////////////////////////////////////////////////////////////////////////
    // Tests for std::duration  compile time requirements
    namespace CompileTimeRequirements
    {
        TYPED_TEST(DurationTypedTest, TraitRequirementsSuccess)
        {
            static_assert(AZStd::is_signed<typename TypeParam::rep>::value, "built in helper types for AZStd::chrono::duration requires representation type to be signed");
            static_assert(AZStd::is_integral<typename TypeParam::rep>::value, "built in helper types for AZStd::chrono::duration requires representation type to an integral type");
            static_assert(std::numeric_limits<typename TypeParam::rep>::digits >= TypeParam::required_bits, "representation type does not have the minimum number of required bits");
            static_assert(AZStd::is_same_v<typename TypeParam::period, typename TypeParam::expected_period>, "duration period type does not match expected period type");
        }
    }
    namespace Comparisons
    {
        TEST_F(DurationTest, Comparisons_SameType)
        {
            constexpr AZStd::chrono::milliseconds threeMillis(3);
            constexpr AZStd::chrono::milliseconds oneMillis(1);
            constexpr AZStd::chrono::milliseconds oneMillisAgain(1);

            static_assert(oneMillis == oneMillisAgain);
            static_assert(threeMillis > oneMillis);
            static_assert(oneMillis < threeMillis);
            static_assert(threeMillis >= oneMillis);
            static_assert(oneMillis <= threeMillis);
            static_assert(threeMillis >= threeMillis);
            static_assert(threeMillis <= threeMillis);
        }
        TEST_F(DurationTest, Comparisons_DifferentType)
        {
            constexpr AZStd::chrono::milliseconds threeMillis(3);
            constexpr AZStd::chrono::milliseconds oneMillis(1);
            // different types:
            constexpr AZStd::chrono::microseconds threeMillisButInMicroseconds(3000);
            constexpr AZStd::chrono::microseconds oneMilliButInMicroseconds(1000);

            static_assert(threeMillisButInMicroseconds > oneMillis);
            static_assert(oneMilliButInMicroseconds < threeMillis);
            static_assert(threeMillis == threeMillisButInMicroseconds);
            static_assert(oneMilliButInMicroseconds == oneMillis);
            static_assert(threeMillisButInMicroseconds >= threeMillis);
            static_assert(threeMillisButInMicroseconds <= threeMillis);
            static_assert(threeMillisButInMicroseconds >= oneMillis);
            static_assert(oneMilliButInMicroseconds <= threeMillis);
        }
    }

    // Test for std::duration arithmatic operations
    namespace ArithmaticOperators
    {
        TEST_F(DurationTest, MillisecondsSubtractionResultsNegativeSuccess)
        {
            constexpr AZStd::chrono::milliseconds milliSeconds(3);
            constexpr AZStd::chrono::milliseconds inverseMilliSeconds = -milliSeconds;
            static_assert(milliSeconds.count() == -inverseMilliSeconds.count(), "inverseMilliseconds should be inverse of milliseconds");
            static_assert(inverseMilliSeconds.count() == -3, "inverseMilliseconds value should be negative");
            constexpr auto subtractResultMilliSeconds = inverseMilliSeconds - milliSeconds;
            static_assert(subtractResultMilliSeconds.count() == -6, "subtract result is incorrect");
            AZStd::chrono::milliseconds compoundSubtractMilliSeconds(5);
            compoundSubtractMilliSeconds -= AZStd::chrono::milliseconds(4);
            EXPECT_EQ(compoundSubtractMilliSeconds.count(), 1);
        }

        TEST_F(DurationTest, MillisecondsAdditionWithNegativeSuccess)
        {
            constexpr AZStd::chrono::milliseconds milliSeconds(3);
            constexpr AZStd::chrono::milliseconds negativeMilliSeconds(-4);
            constexpr auto addResultMilliSeconds = negativeMilliSeconds + milliSeconds;
            static_assert(addResultMilliSeconds.count() == -1, "add result is incorrect");
            AZStd::chrono::milliseconds compoundAddMilliSeconds(5);
            compoundAddMilliSeconds += AZStd::chrono::milliseconds(2);
            EXPECT_EQ(compoundAddMilliSeconds.count(),  7);
        }

        TEST_F(DurationTest, NanosecondsMultiplicationWithNegativeSuccess)
        {
            constexpr AZStd::chrono::nanoseconds negativeNanoSeconds(-16);
            constexpr AZStd::chrono::nanoseconds multiplyResultSeconds = negativeNanoSeconds * 3;
            static_assert(multiplyResultSeconds.count() == -48, "multiply result is incorrect");
            AZStd::chrono::nanoseconds compoundMultiplyNanoSeconds(9);
            compoundMultiplyNanoSeconds *= -2;
            EXPECT_EQ(compoundMultiplyNanoSeconds.count(), -18);
        }

        TEST_F(DurationTest, SecondsDivideWithNegativeSuccess)
        {
            constexpr AZStd::chrono::seconds testSeconds(17);
            constexpr AZStd::chrono::seconds negativeTestSeconds(-3);
            constexpr auto divideResultSeconds = testSeconds / negativeTestSeconds;
            static_assert(divideResultSeconds == -5, "divide result is incorrect");
            AZStd::chrono::seconds compoundDivideSeconds(-42);
            compoundDivideSeconds /= -2;
            EXPECT_EQ(compoundDivideSeconds.count(), 21);
        }

        TEST_F(DurationTest, MicrosecondsModOperatorSuccess)
        {
            constexpr AZStd::chrono::microseconds microSeconds(23);
            constexpr AZStd::chrono::microseconds microSecondsDivisor(2);
            constexpr auto modResultMicroSeconds = microSeconds % microSecondsDivisor;
            static_assert(modResultMicroSeconds.count() == 1, "mod result is incorrect");
            AZStd::chrono::microseconds compoundModMicroSeconds(30);
            compoundModMicroSeconds %= 7;
            EXPECT_EQ(compoundModMicroSeconds.count(), 2);
        }

    }

}
