/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/functional_basic.h>
#include <AzCore/std/tuple.h>


namespace UnitTest
{
    class FunctionalBasicTest
        : public LeakDetectionFixture
    {
    };


    namespace Internal
    {
        struct FunctionalOperatorConfig
        {
            template<typename OperandType, typename T, typename U, typename TupleType>
            static void PerformOperation(T&& lhs, U&& rhs, const TupleType& expectedValues)
            {
                // Arithmetic
                EXPECT_EQ(AZStd::get<0>(expectedValues), AZStd::plus<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<1>(expectedValues), AZStd::minus<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<2>(expectedValues), AZStd::multiplies<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<3>(expectedValues), AZStd::divides<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<4>(expectedValues), AZStd::modulus<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<5>(expectedValues), AZStd::negate<OperandType>{}(AZStd::forward<T>(lhs)));
                // Comparison
                EXPECT_EQ(AZStd::get<6>(expectedValues), AZStd::equal_to<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<7>(expectedValues), AZStd::not_equal_to<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<8>(expectedValues), AZStd::greater<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<9>(expectedValues), AZStd::less<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<10>(expectedValues), AZStd::greater_equal<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<11>(expectedValues), AZStd::less_equal<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                // Logical
                EXPECT_EQ(AZStd::get<12>(expectedValues), AZStd::logical_and<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<13>(expectedValues), AZStd::logical_or<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<14>(expectedValues), AZStd::logical_not<OperandType>{}(AZStd::forward<T>(lhs)));
                // Bitwise
                EXPECT_EQ(AZStd::get<15>(expectedValues), AZStd::bit_and<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<16>(expectedValues), AZStd::bit_or<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<17>(expectedValues), AZStd::bit_xor<OperandType>{}(AZStd::forward<T>(lhs), AZStd::forward<U>(rhs)));
                EXPECT_EQ(AZStd::get<18>(expectedValues), AZStd::bit_not<OperandType>{}(AZStd::forward<T>(lhs)));
            }
        };

        struct IntWrapper
        {
            int32_t m_value;
        };

        // arithmetic operators
        int32_t operator+(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value + rhs;
        }
        int32_t operator+(int32_t lhs, IntWrapper rhs)
        {
            return lhs + rhs.m_value;
        }
        int32_t operator-(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value - rhs;
        }
        int32_t operator-(int32_t lhs, IntWrapper rhs)
        {
            return lhs - rhs.m_value;
        }
        int32_t operator*(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value * rhs;
        }
        int32_t operator*(int32_t lhs, IntWrapper rhs)
        {
            return lhs * rhs.m_value;
        }
        int32_t operator/(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value / rhs;
        }
        int32_t operator/(int32_t lhs, IntWrapper rhs)
        {
            return lhs / rhs.m_value;
        }
        int32_t operator%(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value % rhs;
        }
        int32_t operator%(int32_t lhs, IntWrapper rhs)
        {
            return lhs % rhs.m_value;
        }
        int32_t operator-(IntWrapper lhs)
        {
            return -lhs.m_value;
        }
        // comparison operators
        bool operator==(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value == rhs;
        }
        bool operator==(int32_t lhs, IntWrapper rhs)
        {
            return lhs == rhs.m_value;
        }
        bool operator!=(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value != rhs;
        }
        bool operator!=(int32_t lhs, IntWrapper rhs)
        {
            return lhs != rhs.m_value;
        }
        bool operator>(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value > rhs;
        }
        bool operator>(int lhs, IntWrapper rhs)
        {
            return lhs > rhs.m_value;
        }
        bool operator<(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value < rhs;
        }
        bool operator<(int lhs, IntWrapper rhs)
        {
            return lhs < rhs.m_value;
        }
        bool operator>=(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value >= rhs;
        }
        bool operator>=(int lhs, IntWrapper rhs)
        {
            return lhs >= rhs.m_value;
        }
        bool operator<=(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value <= rhs;
        }
        bool operator<=(int lhs, IntWrapper rhs)
        {
            return lhs <= rhs.m_value;
        }
        // logical operators
        bool operator&&(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value && rhs;
        }
        bool operator&&(int lhs, IntWrapper rhs)
        {
            return lhs && rhs.m_value;
        }
        bool operator||(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value || rhs;
        }
        bool operator||(int lhs, IntWrapper rhs)
        {
            return lhs || rhs.m_value;
        }
        bool operator!(IntWrapper lhs)
        {
            return !lhs.m_value;
        }
        // bitwise operators
        int32_t operator&(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value & rhs;
        }
        int32_t operator&(int lhs, IntWrapper rhs)
        {
            return lhs & rhs.m_value;
        }
        int32_t operator|(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value | rhs;
        }
        int32_t operator|(int lhs, IntWrapper rhs)
        {
            return lhs | rhs.m_value;
        }
        int32_t operator^(IntWrapper lhs, int32_t rhs)
        {
            return lhs.m_value ^ rhs;
        }
        int32_t operator^(int lhs, IntWrapper rhs)
        {
            return lhs ^ rhs.m_value;
        }
        int32_t operator~(IntWrapper lhs)
        {
            return ~lhs.m_value;
        }

        void RawTestFunc(int) {};
    }

    TEST_F(FunctionalBasicTest, FunctionalOperators_ReturnsExpectedValue)
    {
        Internal::FunctionalOperatorConfig::PerformOperation<int>(7, 11, AZStd::make_tuple(18, -4, 77, 0, 7, -7, false, true, false, true, false, true, true, true, false, 3, 15, 12, ~7));
        Internal::FunctionalOperatorConfig::PerformOperation<int>(45, 34, AZStd::make_tuple(79, 11, 1530, 1, 11, -45, false, true, true, false, true, false, true, true, false, 32, 47, 15, ~45));
        Internal::FunctionalOperatorConfig::PerformOperation<int>(24, 24, AZStd::make_tuple(48, 0, 576, 1, 0, -24, true, false, false, false, true, true, true, true, false, 24, 24, 0, ~24));
    }

    TEST_F(FunctionalBasicTest, FunctionalOperators_TransparentOperands)
    {
        Internal::FunctionalOperatorConfig::PerformOperation<void>(7, Internal::IntWrapper{ 11 }, AZStd::make_tuple(18, -4, 77, 0, 7, -7, false, true, false, true, false, true, true, true, false, 3, 15, 12, ~7));
        Internal::FunctionalOperatorConfig::PerformOperation<void>(Internal::IntWrapper{ 45 }, 34, AZStd::make_tuple(79, 11, 1530, 1, 11, -45, false, true, true, false, true, false, true, true, false, 32, 47, 15, ~45));
        Internal::FunctionalOperatorConfig::PerformOperation<void>(24, Internal::IntWrapper{ 24 }, AZStd::make_tuple(48, 0, 576, 1, 0, -24, true, false, false, false, true, true, true, true, false, 24, 24, 0, ~24));
    }

    TEST_F(FunctionalBasicTest, DeductionGuide_Compiles)
    {
        AZStd::function rawFuncDeduce(&Internal::RawTestFunc);
        AZStd::function functionObjectDeduce([](int) -> double { return {}; });
    }
}
