/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Preprocessor/Enum.h>

#include <AzCore/UnitTest/TestTypes.h>


namespace UnitTest
{
    AZ_ENUM_CLASS(TestEnum,
                  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
    );

    AZ_ENUM(TestEnumUnscoped,
            X,
            (Y, 5)
    );

    AZ_ENUM_WITH_UNDERLYING_TYPE(TestEnum8, uint8_t,
                                 XX,
                                 (YY, 7), 
                                 ZZ
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(TestCEnum16, int16_t,
                                       (X, -10),
                                       Y, 
                                       Z
    );

    TEST(AzEnumTest, BasicProperties_AreConstexpr)
    {
        // basic properties verifications:
        static_assert(static_cast<int>(TestEnum::A) == 0);
        static_assert(static_cast<int>(TestEnum::Z) == 25);

        // members array:
        static_assert(TestEnumMembers.size() == 26);
        static_assert(TestEnumMembers[0].m_value == TestEnum::A);
        static_assert(TestEnumMembers[0].m_string == "A");
        static_assert(TestEnumMembers[1].m_value == TestEnum::B);
        static_assert(TestEnumMembers[1].m_string == "B");

        // Unscoped-version tests
        static_assert(Y == 5);

        // 'specific underlying type'-version tests
        static_assert(sizeof(TestEnum8) == sizeof(uint8_t));

        static_assert(XX == 0);
        static_assert(YY == 7);
        static_assert(ZZ == 8);
    }

    TEST(AzEnumTest, EnumerationOverEnum_Succeeds)
    {
        auto EnumerateTestEnum = []() constexpr -> bool
        {
            int count = 0;
            for (TestEnumEnumeratorValueAndString enumMember : TestEnumMembers)
            {
                ++count;
            }
            return count == 26;
        };
        static_assert(EnumerateTestEnum());
    }

    TEST(AzEnumTest, FromString_WithFoundString_ReturnsEnumValue)
    {
        // optional is not dereferencable in constexpr context because of addressof
        // therefore we can't verify the returned value in static_assert, but we can verify that the function found something.
        static_assert(FromStringToTestEnum("Y").has_value());
        static_assert(TestEnumNamespace::FromString("X").has_value());
        // value verification by runtime:
        EXPECT_TRUE(*FromStringToTestEnum("Y") == TestEnum::Y);
        EXPECT_TRUE(*FromStringToTestEnumUnscoped("Y") == Y);
        // namespace accessed functions versions
        EXPECT_TRUE(*TestEnumNamespace::FromString("Y") == TestEnum::Y);
        EXPECT_TRUE(*TestEnumUnscopedNamespace::FromString("Y") == Y);
        // and on runtime as well
        EXPECT_TRUE(*FromStringToTestEnumUnscoped("X") == X);
    }

    TEST(AzEnumTest, FromString_WithNotFoundString_ReturnsInvalid)
    {
        static_assert(!FromStringToTestEnum("alien"));  // nullopt in boolean context should evaluate to false
        static_assert(!FromStringToTestEnumUnscoped("alien"));
        // namespace accessed functions versions
        static_assert(!TestEnumNamespace::FromString("alien"));
        static_assert(!TestEnumUnscopedNamespace::FromString("alien"));
    }

    TEST(AzEnumTest, ToString_WithValidEnumOption_ReturnsNonEmptyString)
    {
        static_assert(ToString(TestEnum::X) == "X");
        static_assert(ToString(TestEnum::Y) == "Y");

        static_assert(ToString(TestEnum8::XX) == "XX");
        static_assert(ToString(TestEnum8::YY) == "YY");
    }

    TEST(AzEnumTest, ToString_WithInvalidEnumValue_ReturnsEmptyString)
    {
        static_assert(ToString(static_cast<TestEnum>(40)).empty());
        static_assert(ToString(static_cast<TestEnumUnscoped>(15)).empty());
        static_assert(ToString(static_cast<TestEnum8>(20)).empty());
    }

    TEST(AzEnumTest, TraitsBehaviorRegular)
    {
        static_assert(AzEnumTraits<TestEnum>::Members.size() == 26);
    }
}
