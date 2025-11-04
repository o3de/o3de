/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/utility/expected.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class ExpectedTest
        : public LeakDetectionFixture
    {
    };

    TEST_F(ExpectedTest, Constructors_CanConstructValueAndError)
    {
        {
            using TestExpected = AZStd::expected<int, AZStd::fixed_string<32>>;
            using VoidExpected = AZStd::expected<void, AZStd::fixed_string<32>>;
            TestExpected testExpected{};
            VoidExpected voidExpected{};
            EXPECT_TRUE(testExpected.has_value());
            EXPECT_TRUE(voidExpected.has_value());
        }
        {
            using TestExpected = AZStd::expected<int, AZStd::fixed_string<32>>;
            using VoidExpected = AZStd::expected<void, AZStd::fixed_string<32>>;
            TestExpected testExpected{ AZStd::in_place, 1 };
            VoidExpected voidExpected{ AZStd::in_place };
            EXPECT_TRUE(testExpected.has_value());
            EXPECT_EQ(1, testExpected.value());
            EXPECT_TRUE(voidExpected.has_value());
        }
        {
            using TestExpected = AZStd::expected<int, AZStd::fixed_string<32>>;
            using VoidExpected = AZStd::expected<void, AZStd::fixed_string<32>>;
            TestExpected testExpected{ AZStd::unexpect };
            VoidExpected voidExpected{ AZStd::unexpect, "Error" };
            EXPECT_FALSE(testExpected.has_value());
            EXPECT_TRUE(testExpected.error().empty());
            EXPECT_FALSE(voidExpected.has_value());
            EXPECT_EQ("Error", voidExpected.error());
        }
        {
            using TestExpected = AZStd::expected<int, AZStd::fixed_string<32>>;
            using InputTestExpected = AZStd::expected<short, AZStd::fixed_string<32>>;
            using VoidExpected = AZStd::expected<void, AZStd::fixed_string<32>>;
            using InputVoidExpected = AZStd::expected<void, AZStd::fixed_string<32>>;

            {
                InputTestExpected inputTestExpected{ short(2) };
                TestExpected testExpected(inputTestExpected);
                EXPECT_TRUE(testExpected.has_value());
                EXPECT_EQ(2, testExpected.value());
            }
            {
                InputTestExpected inputTestExpected{ short(3) };
                TestExpected testExpected{ AZStd::move(inputTestExpected) };
                EXPECT_TRUE(testExpected.has_value());
                EXPECT_EQ(3, testExpected.value());
            }
            {
                InputTestExpected inputTestExpected{ AZStd::unexpect, "Error" };
                TestExpected testExpected{ inputTestExpected };
                EXPECT_FALSE(testExpected.has_value());
                EXPECT_EQ("Error", testExpected.error());
                EXPECT_EQ("Error", inputTestExpected.error());
            }
            {
                InputTestExpected inputTestExpected{ AZStd::unexpect, "Error" };
                TestExpected testExpected{ AZStd::move(inputTestExpected) };
                EXPECT_FALSE(testExpected.has_value());
                EXPECT_EQ("Error", testExpected.error());
                EXPECT_TRUE(inputTestExpected.error().empty());
            }
            {
                InputVoidExpected inputVoidExpected{ AZStd::unexpect, "Error" };
                VoidExpected voidExpected{ inputVoidExpected };
                EXPECT_FALSE(voidExpected.has_value());
                EXPECT_EQ("Error", voidExpected.error());
                EXPECT_EQ("Error", inputVoidExpected.error());
            }
            {
                InputVoidExpected inputVoidExpected{ AZStd::unexpect, "Error" };
                VoidExpected voidExpected{ AZStd::move(inputVoidExpected) };
                EXPECT_FALSE(voidExpected.has_value());
                EXPECT_EQ("Error", voidExpected.error());
                EXPECT_TRUE(inputVoidExpected.error().empty());
            }
        }
        {
            using TestExpected = AZStd::expected<int, AZStd::fixed_string<32>>;
            AZStd::unexpected<AZStd::fixed_string<32>> errorValue{ "Error" };
            TestExpected testExpected{ errorValue };
            EXPECT_FALSE(testExpected.has_value());
            EXPECT_EQ("Error", testExpected.error());
            EXPECT_EQ("Error", errorValue.error());
        }
        {
            using TestExpected = AZStd::expected<int, AZStd::fixed_string<32>>;
            AZStd::unexpected<AZStd::fixed_string<32>> errorValue{ "Error" };
            TestExpected testExpected{ AZStd::move(errorValue) };
            EXPECT_FALSE(testExpected.has_value());
            EXPECT_EQ("Error", testExpected.error());
            EXPECT_TRUE(errorValue.error().empty());
        }
    }

    TEST_F(ExpectedTest, Constructors_ExplicitConstructorsTest)
    {
        // This validates the explicit constructors of AZStd::expected
        // The AZStd::fixed_string<N> template has an explicit constructor which accepts
        // a string_view like type, so an AZStd::string_view is used
        using TestExpected = AZStd::expected<AZStd::fixed_string<32>, AZStd::fixed_string<32>>;
        using ExplicitExpected = AZStd::expected<AZStd::string_view, AZStd::string_view>;
        using ExplicitUnexpected = AZStd::unexpected<AZStd::string_view>;

        ExplicitExpected explicitExpectedValue{ "Hello World" };
        ExplicitExpected explicitExpectedError{ AZStd::unexpect, "Error" };
        ExplicitUnexpected explicitUnexpected{ "Error2" };
        AZStd::string_view testValue{ "Goodbye World" };
        {
            TestExpected testExpected{ explicitExpectedValue };
            EXPECT_TRUE(testExpected.has_value());
            EXPECT_EQ("Hello World", testExpected.value());
            EXPECT_EQ("Hello World", explicitExpectedValue.value());
        }
        {
            TestExpected testExpected{ AZStd::move(explicitExpectedValue) };
            EXPECT_TRUE(testExpected.has_value());
            EXPECT_EQ("Hello World", testExpected.value());
        }
        {
            TestExpected testExpected{ explicitExpectedError };
            EXPECT_FALSE(testExpected.has_value());
            EXPECT_EQ("Error", testExpected.error());
            EXPECT_EQ("Error", explicitExpectedError.error());
        }
        {
            TestExpected testExpected{ AZStd::move(explicitExpectedError) };
            EXPECT_FALSE(testExpected.has_value());
            EXPECT_EQ("Error", testExpected.error());
        }
        {
            TestExpected testExpected{ explicitUnexpected };
            EXPECT_FALSE(testExpected.has_value());
            EXPECT_EQ("Error2", testExpected.error());
            EXPECT_EQ("Error2", explicitUnexpected.error());
        }
        {
            TestExpected testExpected{ AZStd::move(explicitUnexpected) };
            EXPECT_FALSE(testExpected.has_value());
            EXPECT_EQ("Error2", testExpected.error());
        }
        {
            TestExpected testExpected{ testValue };
            EXPECT_TRUE(testExpected.has_value());
            EXPECT_EQ("Goodbye World", testExpected.value());
            EXPECT_EQ("Goodbye World", testValue);
        }
    }

    TEST_F(ExpectedTest, Assignment_CanAssignValueAndError)
    {
        {
            // copy and move assignment of value
            using TestExpected = AZStd::expected<int, AZStd::fixed_string<32>>;
            TestExpected testExpected{ 1 };
            TestExpected resultExpected;
            resultExpected = testExpected;
            EXPECT_TRUE(resultExpected.has_value());
            EXPECT_EQ(1, resultExpected.value());

            resultExpected = AZStd::move(testExpected);
            EXPECT_TRUE(resultExpected.has_value());
            EXPECT_EQ(1, resultExpected.value());
        }
        {
            // copy and move assignment of error
            using TestExpected = AZStd::expected<int, AZStd::fixed_string<32>>;
            TestExpected testExpected{ AZStd::unexpect, "Error"};
            TestExpected resultExpected;
            resultExpected = testExpected;
            EXPECT_FALSE(resultExpected.has_value());
            EXPECT_EQ("Error", resultExpected.error());
            EXPECT_EQ("Error", testExpected.error());

            resultExpected = AZStd::move(testExpected);
            EXPECT_FALSE(resultExpected.has_value());
            EXPECT_EQ("Error", resultExpected.error());
            EXPECT_TRUE(testExpected.error().empty());
        }
        {
            // direct initialize assignment of value
            using TestExpected = AZStd::expected<int, AZStd::fixed_string<32>>;
            
            TestExpected resultExpected;
            resultExpected = 4;
            EXPECT_TRUE(resultExpected.has_value());
            EXPECT_EQ(4, resultExpected.value());
        }
        {
            // direct initialize assignment of error
            using TestExpected = AZStd::expected<int, AZStd::fixed_string<32>>;

            AZStd::unexpected<AZStd::fixed_string<32>> errorValue{ "Error" };
            TestExpected resultExpected;
            resultExpected = errorValue;
            EXPECT_FALSE(resultExpected.has_value());
            EXPECT_EQ("Error", resultExpected.error());
            EXPECT_EQ("Error", errorValue.error());

            resultExpected = AZStd::move(errorValue);
            EXPECT_FALSE(resultExpected.has_value());
            EXPECT_EQ("Error", resultExpected.error());
            EXPECT_TRUE(errorValue.error().empty());
        }
    }

    TEST_F(ExpectedTest, Emplace_CanAssignValue)
    {
        using TestExpected = AZStd::expected<AZStd::fixed_string<32>, AZStd::fixed_string<32>>;
        TestExpected testExpected;
        testExpected.emplace("Hello World");
        EXPECT_TRUE(testExpected.has_value());
        EXPECT_EQ("Hello World", testExpected.value());

        testExpected.emplace({ 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd' });
        EXPECT_TRUE(testExpected.has_value());
        EXPECT_EQ("Hello World", testExpected.value());
    }

    TEST_F(ExpectedTest, OperatorArrow_CanAccessValue_Succeeds)
    {
        using TestExpected = AZStd::expected<AZStd::fixed_string<32>, AZStd::fixed_string<32>>;
        TestExpected testExpected;
        testExpected->assign("Hello World");
        EXPECT_EQ("Hello World", testExpected.value());
    }

    TEST_F(ExpectedTest, OperatorAsterisk_CanAccessValue_Succeeds)
    {
        using TestExpected = AZStd::expected<AZStd::fixed_string<32>, AZStd::fixed_string<32>>;
        TestExpected testExpected;
        *testExpected = "Hello World";
        EXPECT_EQ("Hello World", testExpected.value());
    }

    TEST_F(ExpectedTest, Value_CanModifyValue_Succeeds)
    {
        using TestExpected = AZStd::expected<AZStd::fixed_string<32>, AZStd::fixed_string<32>>;
        TestExpected testExpected;
        testExpected.value() = "Hello World";
        EXPECT_EQ("Hello World", testExpected.value());
    }

    TEST_F(ExpectedTest, Error_CanModifyError_Succeeds)
    {
        using TestExpected = AZStd::expected<AZStd::fixed_string<32>, AZStd::fixed_string<32>>;
        TestExpected testExpected{ AZStd::unexpect };
        testExpected.error() = "Error";
        EXPECT_EQ("Error", testExpected.error());
    }

    TEST_F(ExpectedTest, ValueOr_CanReturnValue_IfHasValue_Or_Default)
    {
        using TestExpected = AZStd::expected<AZStd::fixed_string<32>, AZStd::fixed_string<32>>;
        TestExpected testExpected{ AZStd::unexpect, "Error"};
        EXPECT_FALSE(testExpected);
        EXPECT_EQ("Hello World", testExpected.value_or("Hello World"));
        
        testExpected.emplace("Value");
        EXPECT_TRUE(testExpected);
        EXPECT_EQ("Value", testExpected.value_or("Hello World"));
    }

    TEST_F(ExpectedTest, ComparisonOperator_CanCompareExpected_Value_And_Unexpected)
    {
        using TestExpected = AZStd::expected<AZStd::fixed_string<32>, AZStd::fixed_string<32>>;
        using TestUnexpected = AZStd::unexpected<AZStd::fixed_string<32>>;
        TestExpected testExpectedValue{ "Hello World" };
        TestExpected testExpectedError{ AZStd::unexpect, "Error" };
        TestUnexpected testUnexpected{ "Error2" };
        AZStd::string_view testValue{ "Goodbye World" };

        // compare expected instance to another expected instanct
        EXPECT_EQ(testExpectedValue, testExpectedValue);
        EXPECT_NE(testExpectedValue, testExpectedError);

        // compare expected instance to value
        EXPECT_EQ(testExpectedValue, "Hello World");
        EXPECT_NE(testExpectedValue, testValue);

        // compare expected instance to error
        EXPECT_EQ(testExpectedError, TestUnexpected("Error"));
        EXPECT_NE(testExpectedValue, testUnexpected);
    }

    TEST_F(ExpectedTest, Swap_TwoExpecteds_Succeeds)
    {
        using TestExpected = AZStd::expected<AZStd::fixed_string<32>, AZStd::fixed_string<32>>;
        
        TestExpected testExpectedValue{ "Hello World" };
        TestExpected testExpectedError{ AZStd::unexpect, "Error" };
        AZStd::ranges::swap(testExpectedValue, testExpectedError);

        EXPECT_FALSE(testExpectedValue.has_value());
        EXPECT_EQ("Error", testExpectedValue.error());
        EXPECT_TRUE(testExpectedError.has_value());
        EXPECT_EQ("Hello World", testExpectedError.value());
    }
}
