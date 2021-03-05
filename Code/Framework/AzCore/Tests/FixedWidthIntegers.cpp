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

#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Math/MathUtils.h>

namespace UnitTest
{  
    inline namespace FixedWidthIntegerTestUtils
    {
        // Utility to test each possible comparison between LeftType and RightType.
        // As there are 64 possible integer combinations for L and R to be tested,
        // this utility tests each of the 3 comparison permutations for each 
        // combination as to avoid having 192 nearly identical test cases.
        template<typename LeftType, typename RightType>
        void ValidateSafeComparePermutations()
        {
            // gtest won't pick up on the permutations so print them out for sanity checking
            ColoredPrintf(COLOR_YELLOW, "%s\n",  AZ_FUNCTION_SIGNATURE);

            // Case 1: lhs < rhs
            {
                constexpr LeftType lhs = (std::numeric_limits<LeftType>::lowest)();
                constexpr RightType rhs = (std::numeric_limits<RightType>::max)();

                // Expect lhs to be less than rhs
                static_assert(AZ::IntegralCompare::LessThan == AZ::SafeIntegralCompare(lhs, rhs));
            }
            // Case 2: lhs == rhs
            {
                // Given a lhs and rhs of 0
                constexpr LeftType lhs = 0;
                constexpr RightType rhs = 0;

                // Expect lhs and rhs to be equal
                static_assert(AZ::IntegralCompare::Equal == AZ::SafeIntegralCompare(lhs, rhs));
            }

            // Case 3: lhs > rhs
            {
                // Given a lhs of > 0 and a rhs of <= 0
                constexpr LeftType lhs = (std::numeric_limits<LeftType>::max)();
                constexpr RightType rhs = (std::numeric_limits<RightType>::lowest)();

                // Expect lhs to be greater than rhs
                static_assert(AZ::IntegralCompare::GreaterThan == AZ::SafeIntegralCompare(lhs, rhs));
            }
        }

        // The gtest equality macros do not safely handle integral comparisons between different 
        // signs and widths so we will wrap around them with our own safe compare function in
        // order to test the clamped limits permutations below.

        template<typename LeftType, typename RightType>
        void SAFE_EXPECT_EQ(LeftType lhs, RightType rhs)
        {
            EXPECT_EQ(AZ::IntegralCompare::Equal, AZ::SafeIntegralCompare(lhs, rhs));
        }

        template<typename LeftType, typename RightType>
        void SAFE_EXPECT_LT(LeftType lhs, RightType rhs)
        {
            EXPECT_EQ(AZ::IntegralCompare::LessThan, AZ::SafeIntegralCompare(lhs, rhs));
        }

        template<typename LeftType, typename RightType>
        void SAFE_EXPECT_GT(LeftType lhs, RightType rhs)
        {
            EXPECT_EQ(AZ::IntegralCompare::GreaterThan, AZ::SafeIntegralCompare(lhs, rhs));
        }

        // Below are the different cases for integer combinations used by ValidateClampedLimits for each 
        // possible combination of signedness and size of ValueType and ClampType. 
        // The basic logic behind each case is the same (namely testing the 
        // minimum and maximum clamped range of ValueType for a given ClampType) but the 
        // conditions and expected results are subtley different from one another. As between 
        // these partial specializations and the 64 explicit test cases covering the
        // ClampedIntegralLimits methods every single possible combination of ValueType,
        // ClampType and method call in ClampedIntegralLimits

        template <typename ClampType, typename ValueType>
        constexpr void TestSameSignednessAndEqualSize(ValueType lowestValue, ValueType maxValue)
        {
            // Given:
            // - ValueType is same as ClampType
            // Expect the natural numerical limits of ValueType to be equal to the clamped numerical limits of ValueType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ValueType>::lowest());
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ValueType>::max)());

            // Expect the natural numerical limits of ValueType to be equal to the natural numerical limits of ClampType
            ValueType vMin = AZ::ClampedIntegralLimits<ValueType, ClampType>::Min();
            ClampType cMin = std::numeric_limits<ClampType>::lowest();
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ClampType>::lowest());
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ClampType>::max)());

            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue1, overflow1] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(lowestValue);

            // Expect the original and clamped value to be equal
            SAFE_EXPECT_EQ(clampedValue1, lowestValue);

            // Expect the value to not have overflowed ClampType's limits
            EXPECT_FALSE(overflow1);

            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue2, overflow2] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(maxValue);

            // Expect the original and clamped value to be equal
            SAFE_EXPECT_EQ(clampedValue2, maxValue);

            // Expect the value to not have overflowed ClampType's limits
            EXPECT_FALSE(overflow2);
        }

        template <typename ClampType, typename ValueType>
        constexpr void TestValueTypeSignedAndValueTypeWider(ValueType lowestValue, ValueType maxValue)
        {
            // Given:
            // - ValueType is signed     
            // - ClampType is signed or unsigned
            // - sizeof(ValueType) > sizeof(ClampType)

            // Expect the natural numerical range of ValueType to be larger the clamped numerical range of ValueType
            SAFE_EXPECT_GT(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ValueType>::lowest());
            SAFE_EXPECT_LT(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ValueType>::max)());

            // Expect the clamped numerical limits of ValueType to equal the natural numerical limits of ClampType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ClampType>::lowest());
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ClampType>::max)());

            // Test natural minimum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue1, overflow1] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(lowestValue);

            // Expect the clamped value to be greater than the original value
            SAFE_EXPECT_GT(clampedValue1, lowestValue);

            // Expect the clamped value to equal ClampType's natural min value
            SAFE_EXPECT_EQ(clampedValue1, std::numeric_limits<ClampType>::lowest());

            // Expect the value to have overflowed ClampType's limits
            EXPECT_TRUE(overflow1);

            // Test natural maximum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue2, overflow2] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(maxValue);

            // Expect the clamped value to be less than the original value
            SAFE_EXPECT_LT(clampedValue2, maxValue);

            // Expect the clamped value to equal ClampType's natural min value
            SAFE_EXPECT_EQ(clampedValue2, (std::numeric_limits<ClampType>::max)());

            // Expect the value to have overflowed ClampType's limits
            EXPECT_TRUE(overflow2);
        }

        template <typename ClampType, typename ValueType>
        constexpr void TestClampTypeSignedAndClampTypeWider(ValueType lowestValue, ValueType maxValue)
        {
            // Given:
            // - ValueType is signed or unsigned    
            // - ClampType is signed
            // - sizeof(ValueType) < sizeof(ClampType)

            // Expect the natural numerical limits of ValueType to be equal to the clamped numerical limits of ValueType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ValueType>::lowest());
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ValueType>::max)());

            // Expect the clamped numerical range of ValueType to be smaller than the natural numerical range of ClampType
            SAFE_EXPECT_GT(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ClampType>::lowest());
            SAFE_EXPECT_LT(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ClampType>::max)());

            // Test natural minimum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue1, overflow1] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(lowestValue);

            // Expect the original and clamped value to be equal
            SAFE_EXPECT_EQ(clampedValue1, lowestValue);

            // Expect the value to not have overflowed ClampType's limits
            EXPECT_FALSE(overflow1);

            // Test natural maximum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue2, overflow2] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(maxValue);

            // Expect the original and clamped value to be equal
            SAFE_EXPECT_EQ(clampedValue2, maxValue);

            // Expect the value to not have overflowed ClampType's limits
            EXPECT_FALSE(overflow2);
        }

        template <typename ClampType, typename ValueType>
        constexpr void TestValueTypeSignedClampTypeUnsignedAndClampTypeEqualOrWider(ValueType lowestValue, ValueType maxValue)
        {
            // Given:
            // - ValueType is signed 
            // - ClampType is unsigned
            // - sizeof(ValueType) <= sizeof(ClampType)

            // Expect the natural min value of ValueType to exceed the clamped numerical limits of ValueType
            SAFE_EXPECT_GT(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ValueType>::lowest());

            // Expect the natural max value of ValueType to equal the clamped max value of ValueType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ValueType>::max)());

            // Expect the clamped min value of ValueType to equal the natural min value of ClampType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ClampType>::lowest());

            // Expect the clamped max value of ValueType to be less than the natural max value of ClampType
            SAFE_EXPECT_LT(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ClampType>::max)());

            // Test natural minimum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue1, overflow1] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(lowestValue);
            // Expect the clamped value to be greater than the original value
            SAFE_EXPECT_GT(clampedValue1, lowestValue);

            // Expect the clamped value to equal ClampType's natural min value
            SAFE_EXPECT_EQ(clampedValue1, std::numeric_limits<ClampType>::lowest());

            // Expect the value to have overflowed ClampType's limits
            EXPECT_TRUE(overflow1);

            // Test natural maximum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue2, overflow2] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(maxValue);

            // Expect the original and clamped value to be equal
            SAFE_EXPECT_EQ(clampedValue2, maxValue);

            // Expect the value to not have overflowed ClampType's limits
            EXPECT_TRUE(!overflow2);
        }

        template <typename ClampType, typename ValueType>
        constexpr void TestValueTypeUnsignedClampTypeSignedAndValueTypeEqualOrWider(ValueType lowestValue, ValueType maxValue)
        {
            // Given:
            // - ValueType is unsigned 
            // - ClampType is signed
            // - sizeof(ValueType) >= sizeof(ClampType)

            // Expect the natural min value of ValueType to equal the clamped min value of ValueType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ValueType>::lowest());

            // Expect the natural max value of ValueType to exceed the clamped max value of ValueType
            SAFE_EXPECT_LT(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ValueType>::max)());

            // Expect the clamped min value of ValueType to be greater than the natural min of ClampType
            SAFE_EXPECT_GT(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ClampType>::lowest());

            // Expect the clamped max value of ValueType to equal the natural max of ClampType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ClampType>::max)());

            // Test natural minimum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue1, overflow1] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(lowestValue);

            // Expect the original and clamped value to be equal
            SAFE_EXPECT_EQ(clampedValue1, lowestValue);

            // Expect the value to not have overflowed ClampType's limits
            EXPECT_FALSE(overflow1);

            // Test natural maximum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue2, overflow2] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(maxValue);

            // Expect the clamped value to be less than the original value
            SAFE_EXPECT_LT(clampedValue2, maxValue);

            // Expect the clamped value to equal ClampType's natural min value
            SAFE_EXPECT_EQ(clampedValue2, (std::numeric_limits<ClampType>::max)());

            // Expect the value to have overflowed ClampType's limits
            EXPECT_TRUE(overflow2);
        }

        template <typename ClampType, typename ValueType>
        constexpr void TestUnsignedAndValueTypeWider(ValueType lowestValue, ValueType maxValue)
        {
            // Given:
            // - ValueType is unsigned 
            // - ClampType is unsigned
            // - sizeof(ValueType) > sizeof(ClampType)

            // Expect the natural min value of ValueType to equal the clamped min value of ValueType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ValueType>::lowest());

            // Expect the natural max value of ValueType to exceed the clamped max value of ValueType
            SAFE_EXPECT_LT(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ValueType>::max)());

            // Expect the clamped numerical limits of ValueType to equal the natural numerical limits of ClampType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ClampType>::lowest());
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ClampType>::max)());

            // Test natural minimum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue1, overflow1] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(lowestValue);

            // Expect the original and clamped value to be equal
            SAFE_EXPECT_EQ(clampedValue1, lowestValue);

            // Expect the value to not have overflowed ClampType's limits
            EXPECT_FALSE(overflow1);

            // Test natural maximum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue2, overflow2] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(maxValue);

            // Expect the clamped value to be less than the original value
            SAFE_EXPECT_LT(clampedValue2, maxValue);

            // Expect the clamped value to equal ClampType's natural min value
            SAFE_EXPECT_EQ(clampedValue2, (std::numeric_limits<ClampType>::max)());

            // Expect the value to have overflowed ClampType's limits
            EXPECT_TRUE(overflow2);
        }

        template <typename ClampType, typename ValueType>
        constexpr void TestUnsignedAndClampTypeWider(ValueType lowestValue, ValueType maxValue)
        {
            // Given:
            // - ValueType is unsigned 
            // - ClampType is unsigned
            // - sizeof(ValueType) < sizeof(ClampType)

            // Expect the natural numerical limits of ValueType to be equal to the clamped numerical limits of ValueType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ValueType>::lowest());
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ValueType>::max)());

            // Expect the natural min value of ValueType to equal the clamped min value of ValueType
            SAFE_EXPECT_EQ(AZ::ClampedIntegralLimits<ValueType, ClampType>::Min(), std::numeric_limits<ClampType>::lowest());

            // Expect the natural max value of ValueType to exceed the clamped max value of ValueType
            SAFE_EXPECT_LT(AZ::ClampedIntegralLimits<ValueType, ClampType>::Max(), (std::numeric_limits<ClampType>::max)());

            // Test natural minimum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue1, overflow1] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(lowestValue);

            // Expect the original and clamped value to be equal
            SAFE_EXPECT_EQ(clampedValue1, lowestValue);

            // Expect the value to not have overflowed ClampType's limits
            EXPECT_FALSE(overflow1);

            // Test natural maximum range of ValueType
            // Clamp the value to the natural numerical range of ClampType
            auto[clampedValue2, overflow2] = AZ::ClampedIntegralLimits<ValueType, ClampType>::Clamp(maxValue);

            // Expect the original and clamped value to be equal
            SAFE_EXPECT_EQ(clampedValue2, maxValue);

            // Expect the value to not have overflowed ClampType's limits
            EXPECT_FALSE(overflow2);
        }

        template <typename ValueType, typename ClampType, AZ::IntegralTypeDiff TypeDiff = AZ::IntegralTypeCompare<ValueType, ClampType>()>
        constexpr void IntegralTypeExpectationCompare(ValueType lowestValue, ValueType maxValue)
        {
            if constexpr (TypeDiff == AZ::IntegralTypeDiff::LSignedRSignedEqSize || TypeDiff == AZ::IntegralTypeDiff::LUnsignedRUnsignedEqSize)
            {
                TestSameSignednessAndEqualSize<ClampType>(lowestValue, maxValue);
            }
            else if constexpr (TypeDiff == AZ::IntegralTypeDiff::LSignedRSignedLWider || TypeDiff == AZ::IntegralTypeDiff::LSignedRUnsignedLWider)
            {
                TestValueTypeSignedAndValueTypeWider<ClampType>(lowestValue, maxValue);
            }
            else if constexpr (TypeDiff == AZ::IntegralTypeDiff::LSignedRSignedRWider || TypeDiff == AZ::IntegralTypeDiff::LUnsignedRSignedRWider)
            {
                TestClampTypeSignedAndClampTypeWider<ClampType>(lowestValue, maxValue);
            }
            else if constexpr (TypeDiff == AZ::IntegralTypeDiff::LSignedRUnsignedEqSize || TypeDiff == AZ::IntegralTypeDiff::LSignedRUnsignedRWider)
            {
                TestValueTypeSignedClampTypeUnsignedAndClampTypeEqualOrWider<ClampType>(lowestValue , maxValue);
            }
            else if constexpr (TypeDiff == AZ::IntegralTypeDiff::LUnsignedRSignedEqSize || TypeDiff == AZ::IntegralTypeDiff::LUnsignedRSignedLWider)
            {
                TestValueTypeUnsignedClampTypeSignedAndValueTypeEqualOrWider<ClampType>(lowestValue, maxValue);
            }
            else if constexpr(TypeDiff == AZ::IntegralTypeDiff::LUnsignedRUnsignedLWider)
            {
                TestUnsignedAndValueTypeWider<ClampType>(lowestValue, maxValue);
            }
            else if constexpr(TypeDiff == AZ::IntegralTypeDiff::LUnsignedRUnsignedRWider)
            {
                TestUnsignedAndClampTypeWider<ClampType>(lowestValue, maxValue);
            }
        }

        template<typename ValueType, typename ClampType>
        constexpr void ValidateClampedLimits()
        {
            // gtest won't pick up on the permutations so print them out for sanity checking
            ColoredPrintf(COLOR_YELLOW, "%s\n",  AZ_FUNCTION_SIGNATURE);
            IntegralTypeExpectationCompare<ValueType, ClampType>(std::numeric_limits<ValueType>::lowest(), (std::numeric_limits<ValueType>::max)());
        }
    }

    template<typename ValueType>
    class IntegralTypeTestFixture
        : public ::testing::Test
    {
        
    };

    // gtest has a limit of 50 types, whereas we need 64 (one for each integral combination).
    // We will work around this by only submitting the 8 integral types to gtest but then
    // testing each type against all 8 integral types with a variadic template in order
    // to get the 64 integral type combinations we need for full test coverage.
    template <class... Types>
    struct TypesPack
    {
        template<typename T>
        static void ValidateSafeComparePermutations()
        {
            (FixedWidthIntegerTestUtils::ValidateSafeComparePermutations<T, Types>(), ...);
        }

        template<typename T>
        static void ValidateClampedLimits()
        {
            (FixedWidthIntegerTestUtils::ValidateClampedLimits<T, Types>(), ...);
        }
    };

    // The 8 intergral types that each typed test will test with to from each of the
    // 64 integral type combinations needed for full test coverage.
    using IntegralTypes = TypesPack
    <
        AZ::s8, AZ::u8, AZ::s16, AZ::u16, AZ::s32, AZ::u32, AZ::s64, AZ::u64
    >;

    // The 8 integral types that will form the typed tests.
    using IntegralTypeTestConfigs = ::testing::Types
    <
        AZ::s8, AZ::u8, AZ::s16, AZ::u16, AZ::s32, AZ::u32, AZ::s64, AZ::u64
    >;

    TYPED_TEST_CASE(IntegralTypeTestFixture, IntegralTypeTestConfigs);

    ///////////////////////////////////////////////////////////////////////////

    TYPED_TEST(IntegralTypeTestFixture, SafeCompare)
    {
        IntegralTypes::ValidateSafeComparePermutations<TypeParam>();
    }

    TYPED_TEST(IntegralTypeTestFixture, ClampedLimits)
    {
        IntegralTypes::ValidateClampedLimits<TypeParam>();
    }
}
