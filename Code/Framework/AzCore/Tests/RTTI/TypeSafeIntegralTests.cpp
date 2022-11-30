/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzCore/std/typetraits/is_assignable.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    AZ_TYPE_SAFE_INTEGRAL(TestInt8, int8_t);
    AZ_TYPE_SAFE_INTEGRAL(TestInt8_2, int8_t);
    AZ_TYPE_SAFE_INTEGRAL(TestInt16, int16_t);
    AZ_TYPE_SAFE_INTEGRAL(TestInt32, int32_t);
    AZ_TYPE_SAFE_INTEGRAL(TestInt64, int64_t);
    AZ_TYPE_SAFE_INTEGRAL(TestUInt8, uint8_t);
    AZ_TYPE_SAFE_INTEGRAL(TestUInt16, uint16_t);
    AZ_TYPE_SAFE_INTEGRAL(TestUInt32, uint32_t);
    AZ_TYPE_SAFE_INTEGRAL(TestUInt64, uint64_t);

    template <typename IntegralType>
    class TypeSafeIntegralTests
        : public UnitTest::LeakDetectionFixture
    {
    };

    using IntegralTypes = ::testing::Types<TestInt8, TestInt16, TestInt32, TestInt64, TestUInt8, TestUInt16, TestUInt32, TestUInt64>;
    TYPED_TEST_CASE(TypeSafeIntegralTests, IntegralTypes);

    TYPED_TEST(TypeSafeIntegralTests, TestSum)
    {
        constexpr TypeParam testType1{ 2 };
        constexpr TypeParam testType2{ 1 };
        constexpr TypeParam testType3 = testType1 + testType2;
        static_assert(testType3 == TypeParam{ 3 });
        EXPECT_EQ(testType3, TypeParam{ 3 });

        TypeParam testType4{ 2 };
        TypeParam testType5{ 1 };
        testType4 += testType5;
        EXPECT_EQ(testType4, TypeParam{ 3 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestSub)
    {
        constexpr TypeParam testType1{ 2 };
        constexpr TypeParam testType2{ 1 };
        constexpr TypeParam testType3 = testType1 - testType2;
        static_assert(testType3 == TypeParam{ 1 });
        EXPECT_EQ(testType3, TypeParam{ 1 });

        TypeParam testType4{ 2 };
        TypeParam testType5{ 1 };
        testType4 -= testType5;
        EXPECT_EQ(testType4, TypeParam{ 1 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestMul)
    {
        constexpr TypeParam testType1{ 2 };
        constexpr TypeParam testType2{ 2 };
        constexpr TypeParam testType3 = testType1 * testType2;
        static_assert(testType3 == TypeParam{ 4 });
        EXPECT_EQ(testType3, TypeParam{ 4 });

        TypeParam testType4{ 2 };
        TypeParam testType5{ 2 };
        testType4 *= testType5;
        EXPECT_EQ(testType4, TypeParam{ 4 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestDiv)
    {
        constexpr TypeParam testType1{ 2 };
        constexpr TypeParam testType2{ 2 };
        constexpr TypeParam testType3 = testType1 / testType2;
        static_assert(testType3 == TypeParam{ 1 });
        EXPECT_EQ(testType3, TypeParam{ 1 });

        TypeParam testType4{ 2 };
        TypeParam testType5{ 2 };
        testType4 /= testType5;
        EXPECT_EQ(testType4, TypeParam{ 1 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestMod)
    {
        constexpr TypeParam testType1{ 2 };
        constexpr TypeParam testType2{ 2 };
        constexpr TypeParam testType3 = testType1 % testType2;
        static_assert(testType3 == TypeParam{ 0 });
        EXPECT_EQ(testType3, TypeParam{ 0 });

        TypeParam testType4{ 2 };
        TypeParam testType5{ 2 };
        testType4 %= testType5;
        EXPECT_EQ(testType4, TypeParam{ 0 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestShiftLeft)
    {
        constexpr TypeParam testType1{ 2 };
        constexpr TypeParam testType2 = testType1 << 1;
        static_assert(testType2 == TypeParam{ 4 });
        EXPECT_EQ(testType2, TypeParam{ 4 });

        TypeParam testType3{ 2 };
        testType3 <<= 1;
        EXPECT_EQ(testType3, TypeParam{ 4 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestShiftRight)
    {
        constexpr TypeParam testType1{ 2 };
        constexpr TypeParam testType2 = testType1 >> 1;
        static_assert(testType2 == TypeParam{ 1 });
        EXPECT_EQ(testType2, TypeParam{ 1 });

        TypeParam testType3{ 2 };
        testType3 >>= 1;
        EXPECT_EQ(testType3, TypeParam{ 1 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestOr)
    {
        constexpr TypeParam testType1{ 1 };
        constexpr TypeParam testType2{ 2 };
        constexpr TypeParam testType3 = testType1 | testType2;
        static_assert(testType3 == TypeParam{ 3 });
        EXPECT_EQ(testType3, TypeParam{ 3 });

        TypeParam testType4{ 1 };
        TypeParam testType5{ 2 };
        testType4 |= testType5;
        EXPECT_EQ(testType4, TypeParam{ 3 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestAnd)
    {
        constexpr TypeParam testType1{ 1 };
        constexpr TypeParam testType2{ 3 };
        constexpr TypeParam testType3 = testType1 & testType2;
        static_assert(testType3 == TypeParam{ 1 });
        EXPECT_EQ(testType3, TypeParam{ 1 });

        TypeParam testType4{ 1 };
        TypeParam testType5{ 3 };
        testType4 &= testType5;
        EXPECT_EQ(testType4, TypeParam{ 1 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestXor)
    {
        constexpr TypeParam testType1{ 1 };
        constexpr TypeParam testType2{ 3 };
        constexpr TypeParam testType3 = testType1 ^ testType2;
        static_assert(testType3 == TypeParam{ 2 });
        EXPECT_EQ(testType3, TypeParam{ 2 });

        TypeParam testType4{ 1 };
        TypeParam testType5{ 3 };
        testType4 ^= testType5;
        EXPECT_EQ(testType4, TypeParam{ 2 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestComparisons)
    {
        constexpr TypeParam testType1{ 1 };
        constexpr TypeParam testType2{ 2 };
        constexpr TypeParam testType3{ 2 };
        constexpr TypeParam testType4{ 3 };

        static_assert( (testType1 <  testType2));
        static_assert( (testType3 <  testType4));
        static_assert(!(testType1 >  testType2));
        static_assert(!(testType3 >  testType4));
        static_assert(!(testType1 >= testType2));
        static_assert(!(testType3 >= testType4));
        static_assert( (testType2 <= testType3));
        static_assert( (testType2 >= testType3));
        EXPECT_TRUE (testType1 <  testType2);
        EXPECT_TRUE (testType3 <  testType4);
        EXPECT_FALSE(testType1 >  testType2);
        EXPECT_FALSE(testType3 >  testType4);
        EXPECT_FALSE(testType1 >= testType2);
        EXPECT_FALSE(testType3 >= testType4);
        EXPECT_TRUE (testType2 <= testType3);
        EXPECT_TRUE (testType2 >= testType3);

        static_assert( (testType2 >  testType1));
        static_assert( (testType4 >  testType3));
        static_assert(!(testType2 <  testType1));
        static_assert(!(testType4 <  testType3));
        static_assert(!(testType2 <= testType1));
        static_assert(!(testType4 <= testType3));
        static_assert( (testType3 >= testType2));
        static_assert( (testType3 <= testType2));
        EXPECT_TRUE (testType2 >  testType1);
        EXPECT_TRUE (testType4 >  testType3);
        EXPECT_FALSE(testType2 <  testType1);
        EXPECT_FALSE(testType4 <  testType3);
        EXPECT_FALSE(testType2 <= testType1);
        EXPECT_FALSE(testType4 <= testType3);
        EXPECT_TRUE (testType3 >= testType2);
        EXPECT_TRUE (testType3 <= testType2);

        static_assert(!(testType2 == testType1));
        static_assert(!(testType4 == testType3));
        static_assert( (testType2 == testType3));
        static_assert( (testType4 == testType4));
        static_assert( (testType2 != testType1));
        static_assert( (testType4 != testType3));
        static_assert(!(testType2 != testType3));
        static_assert(!(testType4 != testType4));
        EXPECT_FALSE(testType2 == testType1);
        EXPECT_FALSE(testType4 == testType3);
        EXPECT_TRUE (testType2 == testType3);
        EXPECT_TRUE (testType4 == testType4);
        EXPECT_TRUE (testType2 != testType1);
        EXPECT_TRUE (testType4 != testType3);
        EXPECT_FALSE(testType2 != testType3);
        EXPECT_FALSE(testType4 != testType4);
    }

    TYPED_TEST(TypeSafeIntegralTests, TestIncrement)
    {
        TypeParam testType1{1};
        TypeParam testType2 = ++testType1;
        EXPECT_EQ(testType1, TypeParam{ 2 });
        EXPECT_EQ(testType2, TypeParam{ 2 });

        TypeParam testType3 = testType1++;
        EXPECT_EQ(testType3, TypeParam{ 2 });
        EXPECT_EQ(testType1, TypeParam{ 3 });
    }

    TYPED_TEST(TypeSafeIntegralTests, TestDecrement)
    {
        TypeParam testType1{ 3 };
        TypeParam testType2 = --testType1;
        EXPECT_EQ(testType1, TypeParam{ 2 });
        EXPECT_EQ(testType2, TypeParam{ 2 });

        TypeParam testType3 = testType1--;
        EXPECT_EQ(testType3, TypeParam{ 2 });
        EXPECT_EQ(testType1, TypeParam{ 1 });
    }

    TEST(TypeSafeIntegralTests, TestAssignable)
    {
        static_assert(std::is_assignable<TestInt8&, TestInt8>::value, "Same types should be assignable");
        static_assert(std::is_assignable<TestInt16&, TestInt16>::value, "Same types should be assignable");
        static_assert(std::is_assignable<TestInt32&, TestInt32>::value, "Same types should be assignable");
        static_assert(std::is_assignable<TestInt64&, TestInt64>::value, "Same types should be assignable");
    }

    TEST(TypeSafeIntegralTests, TestNotAssignable)
    {
        // Test that we can't assign different types
        static_assert(!std::is_assignable<TestInt8&, TestInt16>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt8&, TestInt32>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt8&, TestInt64>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt16&, TestInt8>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt16&, TestInt32>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt16&, TestInt64>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt32&, TestInt8>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt32&, TestInt16>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt32&, TestInt64>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt64&, TestInt8>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt64&, TestInt16>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt64&, TestInt32>::value, "Different types should not be assignable");

        // Test that we can't assign different types even if the underlying type is identical
        static_assert(!std::is_assignable<TestInt8&, TestInt8_2>::value, "Different types should not be assignable");
        static_assert(!std::is_assignable<TestInt8_2&, TestInt8>::value, "Different types should not be assignable");
    }
}
