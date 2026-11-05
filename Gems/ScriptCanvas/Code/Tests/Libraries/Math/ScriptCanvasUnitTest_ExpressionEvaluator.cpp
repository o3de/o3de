/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <ScriptCanvas/Utils/Expression.h>

#include <cctype>
#include <cmath>
#include <limits>
#include <numbers>
#include <ostream>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas::Expression;

    class ExpressionEvaluatorTests : public UnitTest::LeakDetectionFixture
    {
    protected:
        static Result<Program> Parse(const AZStd::string& source, bool bracedVariables = true)
        {
            const auto tokens = Tokenize(source, bracedVariables);
            if (!tokens)
            {
                return { {}, tokens.m_error };
            }
            return ShuntingYard(tokens.m_value);
        }

        static Result<double> EvaluateSource(const AZStd::string& source, const Bindings& bindings = {})
        {
            const auto program = Parse(source);
            if (!program)
            {
                return { {}, program.m_error };
            }
            const auto values = Bind(program.m_value, bindings);
            if (!values)
            {
                return { {}, values.m_error };
            }
            return Evaluate(program.m_value, values.m_value);
        }
    };

    struct ExpressionCase
    {
        const char* m_source;
        double m_expected;
    };

    void PrintTo(const ExpressionCase& test, std::ostream* stream)
    {
        *stream << test.m_source;
    }

    class ExpressionValueTests
        : public ExpressionEvaluatorTests
        , public ::testing::WithParamInterface<ExpressionCase>
    {
    };

    TEST_P(ExpressionValueTests, EvaluatesExpectedValue)
    {
        const auto& [source, expected] = GetParam();
        SCOPED_TRACE(source);
        const auto result = EvaluateSource(source);
        ASSERT_TRUE(result) << result.m_error.c_str();
        EXPECT_NEAR(result.m_value, expected, expected == 0 ? 1e-12 : 1e-12 * std::abs(expected));
    }

    const ExpressionCase LegacyExpressions[] = {
        { "3.0+1.0", 4 },
        { "1.0+3.0", 4 },
        { "3.0-1.0", 2 },
        { "1.0-3.0", -2 },
        { "2.0*3.0", 6 },
        { "3.0*2.0", 6 },
        { "4.0/2.0", 2 },
        { "2.0/4.0", 0.5 },
        { "4.0%2.0", 0 },
        { "2.0%4.0", 2 },
        { "3+1", 4 },
        { "3+1*5", 8 },
        { "(3 + 1)", 4 },
        { "((2.0 + 2.0)*(4.0 - 3.0)) / 2.0", 2 },
        { "((   ((   )  )(       2.0) + 2.0)*(4.0 - 3.0)) / (((2.0              )))", 2 },
        { "()1", 1 },
        { "(()())2", 2 },
        { "(())(2+3)", 5 },
        { "((())) 3 + 1", 4 },
        { "(3 + 1) % 2", 0 },
        { "123", 123 },
        { "0.12", 0.12 },
        { "true", 1 },
        { "tRuE", 1 },
        { "TRUE", 1 },
        { "false", 0 },
        { "FaLsE", 0 },
        { "FALSE", 0 },
        { "true || false", 1 },
        { "2+2 || 1+3", 1 },
    };

    INSTANTIATE_TEST_SUITE_P(LegacyGem, ExpressionValueTests, ::testing::ValuesIn(LegacyExpressions));

    const ExpressionCase ArithmeticExpressions[] = {
        { "2+3*4-5/2", 11.5 },
        { "10-3-2", 5 },
        { "20/5/2", 2 },
        { "20/5*2", 8 },
        { "2^3^2", 512 },
        { "2**3**2", 512 },
        { "2^3**2", 512 },
        { "2**3^2", 512 },
        { "(2^3)^2", 64 },
        { "-2^2", -4 },
        { "(-2)^2", 4 },
        { "2^-2", 0.25 },
        { "-2^-2", -0.25 },
        { "2^-2^2", 0.0625 },
        { "2^+3", 8 },
        { "--2 + +3", 5 },
        { "1+-2", -1 },
        { "1--2", 3 },
        { "3*-2", -6 },
        { "3/-2", -1.5 },
        { "1<2 && 2<=2 && 3>2 && 3>=3", 1 },
        { "1==2 || 2!=3", 1 },
        { "2<1", 0 },
        { "2<=1", 0 },
        { "2>3", 0 },
        { "2>=3", 0 },
        { "2==2", 1 },
        { "2!=2", 0 },
        { "1+2>2==true", 1 },
        { "1<2==3<4", 1 },
        { "false || true && false", 0 },
        { "(false || true) && false", 0 },
        { "true || false && false", 1 },
        { "-2 && 3", 1 },
        { "0 || -0.5", 1 },
        { "!0 + !2", 1 },
        { "!!5", 1 },
        { "!2^0", 0 },
        { "2^!0", 2 },
        { "(1<2)+(2>3)", 1 },
        { "5/2", 2.5 },
        { "1/4+1/8", 0.375 },
        { "16777217-16777216", 1 },
        { "9007199254740992+1==9007199254740992", 1 },
    };

    INSTANTIATE_TEST_SUITE_P(ArithmeticAndPrecedence, ExpressionValueTests, ::testing::ValuesIn(ArithmeticExpressions));

    const ExpressionCase RemainderExpressions[] = {
        { "5%2", 1 },         { "5.5%2", 1.5 },    { "5%2.5", 0 },           { "5.5%2.25", 1 }, { "-5.5%2", -1.5 },
        { "5.5%-2", 1.5 },    { "-5.5%-2", -1.5 }, { "-1%3", -1 },           { "1%-3", 1 },     { "0%2.5", 0 },
        { "0.75%0.5", 0.25 }, { "1e20%3", 1 },     { "1e-10%3e-11", 1e-11 }, { "20%6%3", 2 },   { "20/3%2", 2.0 / 3.0 },
        { "20%6*2", 4 },      { "20*3%7", 4 },     { "2+7%4*3", 11 },        { "2^3%3", 2 },    { "(2+7)%4", 1 },
        { "(7%4)^2", 9 },
    };

    INSTANTIATE_TEST_SUITE_P(Remainders, ExpressionValueTests, ::testing::ValuesIn(RemainderExpressions));

    const ExpressionCase FunctionExpressions[] = {
        { "pi", std::numbers::pi },
        { "e", std::numbers::e },
        { "sin(pi/6)", 0.5 },
        { "cos(pi/3)", 0.5 },
        { "tan(pi/4)", 1 },
        { "asin(0.5)", std::numbers::pi / 6 },
        { "acos(0.5)", std::numbers::pi / 3 },
        { "atan(1)", std::numbers::pi / 4 },
        { "atan2(1,0)", std::numbers::pi / 2 },
        { "atan2(-1,-1)", -3 * std::numbers::pi / 4 },
        { "sinh(log(2))", 0.75 },
        { "cosh(log(2))", 1.25 },
        { "tanh(log(2))", 0.6 },
        { "asinh(0.75)", std::numbers::ln2 },
        { "acosh(1.25)", std::numbers::ln2 },
        { "atanh(0.6)", std::numbers::ln2 },
        { "sqrt(9)", 3 },
        { "cbrt(-8)", -2 },
        { "exp(1)", std::numbers::e },
        { "exp2(-2)", 0.25 },
        { "log(e)", 1 },
        { "ln(e)", 1 },
        { "log2(8)", 3 },
        { "log10(100)", 2 },
        { "abs(-2.25)", 2.25 },
        { "floor(-1.25)", -2 },
        { "ceil(-1.25)", -1 },
        { "floor(1.25)", 1 },
        { "ceil(1.25)", 2 },
        { "round(1.5)", 2 },
        { "round(-1.5)", -2 },
        { "trunc(-1.9)", -1 },
        { "trunc(1.9)", 1 },
        { "pow(2,-3)", 0.125 },
        { "hypot(3,4)", 5 },
        { "min(-2,-3)", -3 },
        { "max(-2,-3)", -2 },
        { "max(1,min(3,pow(2,3)))+hypot(3,4)", 8 },
        { "sin(cos(0)*pi/2)", 1 },
    };

    INSTANTIATE_TEST_SUITE_P(Functions, ExpressionValueTests, ::testing::ValuesIn(FunctionExpressions));

    const ExpressionCase FormattedExpressions[] = {
        { "0", 0 },
        { "-0", 0 },
        { ".5", 0.5 },
        { "1.", 1 },
        { "00012", 12 },
        { "000.125", 0.125 },
        { "1e3", 1000 },
        { "1E+03", 1000 },
        { "1.e2", 100 },
        { ".5e+2", 50 },
        { "1e-3", 0.001 },
        { ".5 + 1. + 1e-3 + 2E+1", 21.501 },
        { "\t\n\r\v\f 2 \t+\n 3\r ", 5 },
        { "sin \t( pi \n/ 2 )", 1 },
        { "max \n( 1 \t,\r min ( 2 , 3 ) )", 2 },
        { "- \t - \n 2", 2 },
        { "2 * - \t 3", -6 },
        { "2 \t** \n3", 8 },
        { "[ 2 + 3 ] * ( 4 - 1 )", 15 },
        { "sin[pi/2]", 1 },
        { "max[1,2]", 2 },
        { "1 \n?\t 2\r :\f 3", 2 },
        { "1.7976931348623157e308", std::numeric_limits<double>::max() },
        { "2.2250738585072014e-308", std::numeric_limits<double>::min() },
        { "1e-308", 1e-308 },
        { "4.9406564584124654e-324", std::numeric_limits<double>::denorm_min() },
        { "1e-200*1e200", 1 },
        { "1e-200*1e-200", 0 },
    };

    INSTANTIATE_TEST_SUITE_P(NumberFormatsAndWhitespace, ExpressionValueTests, ::testing::ValuesIn(FormattedExpressions));

    const ExpressionCase ConditionalExpressions[] = {
        { "1?2:3", 2 },
        { "0?2:3", 3 },
        { "-1?2:3", 2 },
        { "0.125?2:3", 2 },
        { "-0.0?2:3", 3 },
        { "1||0?2:3", 2 },
        { "0&&1?2:3", 3 },
        { "1?0||1:2", 1 },
        { "0?2:0||1", 1 },
        { "0?1:2+3*4", 14 },
        { "1+1?-2^2:9", -4 },
        { "1?2:0?4:5", 2 },
        { "0?2:1?4:5", 4 },
        { "0?2:0?4:5", 5 },
        { "1?0?2:3:4", 3 },
        { "1?1?2:3:4", 2 },
        { "0?1?2:3:4", 4 },
        { "(1?0:1)?2:3", 3 },
        { "(0?0:1)?2:3", 2 },
        { "(1?2:3)+4", 6 },
        { "4*(0?2:3)", 12 },
        { "max(1?2:3,0?4:5)", 5 },
        { "pow(0?2:3,1?2:4)", 9 },
        { "1?max(2,3):min(4,5)", 3 },
        { "sin(1?pi/2:0)", 1 },
        { "1?7:1/0", 7 },
        { "0?sqrt(-1):8", 8 },
        { "1?0?1/0:6:sqrt(-1)", 6 },
        { "0?1/0:0?sqrt(-1):7", 7 },
        { "0?1%0:9", 9 },
        { "1?9:exp(1000)", 9 },
    };

    INSTANTIATE_TEST_SUITE_P(Ternaries, ExpressionValueTests, ::testing::ValuesIn(ConditionalExpressions));

    class ExpressionParseErrorTests
        : public ExpressionEvaluatorTests
        , public ::testing::WithParamInterface<const char*>
    {
    };

    TEST_P(ExpressionParseErrorTests, RejectsMalformedExpression)
    {
        SCOPED_TRACE(GetParam());
        const auto result = Parse(GetParam());
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.m_error.empty());
    }

    INSTANTIATE_TEST_SUITE_P(
        Syntax,
        ExpressionParseErrorTests,
        ::testing::Values(
            "",
            " \t\n\r\v\f",
            ".",
            "1..2",
            "1.2.3",
            "1e",
            "1e+",
            "1e-",
            "1 e2",
            "1e -2",
            "1e+ 2",
            "1e309",
            "1e-999",
            "0x10",
            "0X1p2",
            "0b10",
            "1_000",
            "1'000",
            "1.0f",
            "1.0F",
            "1.0L",
            ".5f",
            "2 3",
            "2x",
            "2(3)",
            "(2)3",
            "(2)(3)",
            "x y",
            "{a}{b}",
            "1+",
            "*2",
            "1!",
            "2!3",
            "1=2",
            "a=1?2:3",
            "1&2",
            "1|2",
            "~1",
            "1<<2",
            "1>>2",
            "1 & & 2",
            "1 | | 2",
            "1 < = 2",
            "1 > = 2",
            "1 = = 2",
            "1 ! = 2",
            "2 * * 3",
            "@",
            "1;2",
            "1#2",
            "1//2",
            "1/*2*/3",
            "1,2",
            "1,5+2",
            "()",
            "1+()",
            "(1+())",
            "1()",
            "sin(())",
            "(-)",
            "[]",
            "1+1)",
            "(1+1",
            "[2)",
            "(2]",
            "([2)]",
            "[2",
            "2]",
            "sin()",
            "sin(1,2)",
            "pow(2)",
            "max(1,2,3)",
            "max(,2)",
            "max(1,)",
            "max((1,2),3)",
            "wat(1)",
            "Sin(1)",
            "pi(1)",
            "sin 1",
            "{}",
            "{unclosed",
            "?1:2",
            "1?",
            "1?2",
            "1?2:",
            "1?:2",
            "1:2",
            "1?2:3:4",
            "1??2:3:4",
            "(1?2):3",
            "1?(2:3)",
            "1?2,3:4",
            "max(1?2,3:4)",
            "max(1,2:3)",
            "1?2:unknown(3)",
            "1?2:(3+)"));

    class ExpressionEvaluationErrorTests
        : public ExpressionEvaluatorTests
        , public ::testing::WithParamInterface<const char*>
    {
    };

    TEST_P(ExpressionEvaluationErrorTests, RejectsNonFiniteResultsAndZeroDivisors)
    {
        SCOPED_TRACE(GetParam());
        const auto program = Parse(GetParam());
        ASSERT_TRUE(program) << program.m_error.c_str();
        const auto result = Evaluate(program.m_value, {});
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.m_error.empty());
    }

    INSTANTIATE_TEST_SUITE_P(
        ArithmeticErrors,
        ExpressionEvaluationErrorTests,
        ::testing::Values(
            "2.0/0.0",
            "2.0%0.0",
            "1/-0.0",
            "1%-0.0",
            "0/0",
            "sqrt(-1)",
            "log(0)",
            "log(-1)",
            "ln(0)",
            "log2(-1)",
            "log10(0)",
            "asin(2)",
            "acos(-2)",
            "acosh(0.5)",
            "atanh(1)",
            "atanh(-1)",
            "exp(1000)",
            "exp2(1024)",
            "sinh(1000)",
            "cosh(1000)",
            "pow(-1,0.5)",
            "0^-1",
            "1e308*2",
            "1e308+1e308",
            "1?1/0:2",
            "0?2:sqrt(-1)",
            "0&&1/0",
            "1||sqrt(-1)"));

    TEST_F(ExpressionEvaluatorTests, LegacyVariablesRetainFirstOccurrenceOrderAndCanBeRebound)
    {
        const auto parsed = Parse("{A}+{B}+{A}");
        ASSERT_TRUE(parsed) << parsed.m_error.c_str();
        EXPECT_EQ(parsed.m_value.m_variables, (AZStd::vector<AZStd::string>{ "A", "B" }));
        EXPECT_EQ(parsed.m_value.m_instructions.size(), 5);
        for (const auto& bindings : { Bindings{ { "A", 3.0 }, { "B", 1.0 } }, Bindings{ { "B", -2.0 }, { "A", 0.5 } } })
        {
            const auto values = Bind(parsed.m_value, bindings);
            ASSERT_TRUE(values) << values.m_error.c_str();
            const auto result = Evaluate(parsed.m_value, values.m_value);
            ASSERT_TRUE(result) << result.m_error.c_str();
            EXPECT_DOUBLE_EQ(result.m_value, bindings.at("A") * 2 + bindings.at("B"));
        }
        const auto sum = EvaluateSource("{A}+{B}", { { "A", 3.0 }, { "B", 1.0 } });
        ASSERT_TRUE(sum) << sum.m_error.c_str();
        EXPECT_DOUBLE_EQ(sum.m_value, 4);
        const auto product = EvaluateSource("{Cats}*{Dogs}", { { "Cats", 2.0 }, { "Dogs", 3.0 } });
        ASSERT_TRUE(product) << product.m_error.c_str();
        EXPECT_DOUBLE_EQ(product.m_value, 6);
    }

    TEST_F(ExpressionEvaluatorTests, MixedFloatAndDoubleBindingsUseDoubleArithmetic)
    {
        const float floatValue = 0.1f;
        const double doubleValue = 0.2;
        const Bindings bindings{ { "f", floatValue }, { "d", doubleValue } };
        const ExpressionCase cases[] = {
            { "{f}+{d}", static_cast<double>(floatValue) + doubleValue },
            { "{d}+{f}", doubleValue + static_cast<double>(floatValue) },
            { "{f}-{d}", static_cast<double>(floatValue) - doubleValue },
            { "{d}-{f}", doubleValue - static_cast<double>(floatValue) },
            { "{f}*{d}", static_cast<double>(floatValue) * doubleValue },
            { "{f}/{d}", static_cast<double>(floatValue) / doubleValue },
            { "{d}/{f}", doubleValue / static_cast<double>(floatValue) },
            { "{f}%{d}", std::fmod(static_cast<double>(floatValue), doubleValue) },
            { "{d}%{f}", std::fmod(doubleValue, static_cast<double>(floatValue)) },
            { "min({f},{d})", floatValue },
            { "max({f},{d})", doubleValue },
            { "{f}<{d}?{f}:{d}", floatValue },
        };
        for (const auto& [source, expected] : cases)
        {
            SCOPED_TRACE(source);
            const auto result = EvaluateSource(source, bindings);
            ASSERT_TRUE(result) << result.m_error.c_str();
            EXPECT_DOUBLE_EQ(result.m_value, expected);
        }
        const auto precision = EvaluateSource("{f}+{d}-{f}", { { "f", 16777216.0f }, { "d", 1.0 } });
        ASSERT_TRUE(precision) << precision.m_error.c_str();
        EXPECT_DOUBLE_EQ(precision.m_value, 1);
        const float maximum = std::numeric_limits<float>::max();
        const auto range = EvaluateSource("{f}*{d}", { { "f", maximum }, { "d", 2.0 } });
        ASSERT_TRUE(range) << range.m_error.c_str();
        EXPECT_DOUBLE_EQ(range.m_value, static_cast<double>(maximum) * 2.0);
    }

    TEST_F(ExpressionEvaluatorTests, TokenizerPreservesBoundariesAndLongestMatchingOperators)
    {
        const auto tokens = Tokenize(".5e+2<=+_x2&&{Cats}!=0", true);
        ASSERT_TRUE(tokens) << tokens.m_error.c_str();
        ASSERT_EQ(tokens.m_value.size(), 8);
        EXPECT_EQ(tokens.m_value[0].m_kind, Kind::Number);
        EXPECT_DOUBLE_EQ(tokens.m_value[0].m_value, 50);
        const char* texts[] = { "<=", "+", "_x2", "&&", "Cats", "!=", "" };
        for (size_t i = 0; i < 7; ++i)
        {
            EXPECT_EQ(tokens.m_value[i + 1].m_text, texts[i]);
        }
        EXPECT_EQ(tokens.m_value[3].m_kind, Kind::Name);
        EXPECT_EQ(tokens.m_value[5].m_kind, Kind::Variable);
        EXPECT_EQ(tokens.m_value[7].m_kind, Kind::Number);
        EXPECT_DOUBLE_EQ(tokens.m_value[7].m_value, 0);
        for (const char* op : { "+", "-", "*", "/", "%", "^", "**", "!", "==", "!=", "<", "<=", ">", ">=", "&&", "||" })
        {
            SCOPED_TRACE(op);
            const auto token = Tokenize(op);
            ASSERT_TRUE(token) << token.m_error.c_str();
            ASSERT_EQ(token.m_value.size(), 1);
            EXPECT_EQ(token.m_value[0].m_kind, Kind::Operator);
            EXPECT_EQ(token.m_value[0].m_text, op);
        }
    }

    TEST_F(ExpressionEvaluatorTests, EveryAsciiWhitespaceCombinationPreservesTokenBoundaries)
    {
        for (char before : { ' ', '\t', '\n', '\r', '\v', '\f' })
        {
            for (char after : { ' ', '\t', '\n', '\r', '\v', '\f' })
            {
                AZStd::string source;
                for (const char* token : { "max", "(", ".5e+2", ",", "3", ")", "+", "(", "2", "**", "3", ")", "%", "7" })
                {
                    source += before;
                    source += token;
                    source += after;
                }
                const auto result = EvaluateSource(source);
                ASSERT_TRUE(result) << result.m_error.c_str();
                EXPECT_DOUBLE_EQ(result.m_value, 51);
                EXPECT_FALSE(Parse(AZStd::string("1e") + before + "+2"));
                EXPECT_FALSE(Parse(AZStd::string("1") + before + ".5"));
                EXPECT_FALSE(Parse(AZStd::string("2*") + before + after + "*3"));
            }
        }
    }

    TEST_F(ExpressionEvaluatorTests, LongExpressionsAndDeepGroupingDoNotLoseTokensOrBindings)
    {
        AZStd::string source = "{value}";
        for (int i = 0; i < 1024; ++i)
        {
            source += "+{value}";
        }
        const auto program = Parse(source);
        ASSERT_TRUE(program) << program.m_error.c_str();
        EXPECT_EQ(program.m_value.m_variables.size(), 1);
        EXPECT_EQ(program.m_value.m_instructions.size(), 2049);
        const auto result = Evaluate(program.m_value, { 0.5f });
        ASSERT_TRUE(result) << result.m_error.c_str();
        EXPECT_DOUBLE_EQ(result.m_value, 512.5);

        const AZStd::string nested = AZStd::string(256, '(') + "1?7:1/0" + AZStd::string(256, ')');
        const auto grouped = EvaluateSource(nested);
        ASSERT_TRUE(grouped) << grouped.m_error.c_str();
        EXPECT_DOUBLE_EQ(grouped.m_value, 7);

        const AZStd::string name(1024, 'x');
        const auto variable = EvaluateSource("{" + name + "}+1", { { name, 2.0f } });
        ASSERT_TRUE(variable) << variable.m_error.c_str();
        EXPECT_DOUBLE_EQ(variable.m_value, 3);

        AZStd::string falseBranches;
        AZStd::string trueBranches;
        for (int i = 0; i < 256; ++i)
        {
            falseBranches += "0?1/0:";
            trueBranches += "1?";
        }
        trueBranches += "7";
        for (int i = 0; i < 256; ++i)
        {
            trueBranches += ":1/0";
        }
        for (const auto& conditional : { falseBranches + "7", trueBranches })
        {
            const auto selected = EvaluateSource(conditional);
            ASSERT_TRUE(selected) << selected.m_error.c_str();
            EXPECT_DOUBLE_EQ(selected.m_value, 7);
        }
    }

    TEST_F(ExpressionEvaluatorTests, DecimalLiteralsAndConstantsRetainDoublePrecision)
    {
        const ExpressionCase cases[] = {
            { "0.1", 0.1 },
            { "16777217.0", 16777217.0 },
            { "9007199254740991", 9007199254740991.0 },
            { "1.0000000000000002", 1.0 + std::numeric_limits<double>::epsilon() },
            { "1.7976931348623157e308", std::numeric_limits<double>::max() },
            { "2.2250738585072014e-308", std::numeric_limits<double>::min() },
            { "pi", std::numbers::pi },
            { "e", std::numbers::e },
        };
        for (const auto& [source, expected] : cases)
        {
            SCOPED_TRACE(source);
            const auto result = EvaluateSource(source);
            ASSERT_TRUE(result) << result.m_error.c_str();
            EXPECT_EQ(result.m_value, expected);
        }
    }

    TEST_F(ExpressionEvaluatorTests, BracedNamesPreserveWhitespacePunctuationAndConstantNames)
    {
        const auto parsed = Parse("{pi}+{input with spaces}*{pi}+{ x }+{x}+{a@]]\"}");
        ASSERT_TRUE(parsed) << parsed.m_error.c_str();
        EXPECT_EQ(parsed.m_value.m_variables, (AZStd::vector<AZStd::string>{ "pi", "input with spaces", " x ", "x", "a@]]\"" }));
        const auto values = Bind(parsed.m_value, { { "pi", 2 }, { "input with spaces", 3 }, { " x ", 4 }, { "x", 5 }, { "a@]]\"", 6 } });
        ASSERT_TRUE(values) << values.m_error.c_str();
        const auto result = Evaluate(parsed.m_value, values.m_value);
        ASSERT_TRUE(result) << result.m_error.c_str();
        EXPECT_DOUBLE_EQ(result.m_value, 23);
        const auto constants = EvaluateSource("{true}+{false}+{e}", { { "true", 2 }, { "false", 3 }, { "e", 4 } });
        ASSERT_TRUE(constants) << constants.m_error.c_str();
        EXPECT_DOUBLE_EQ(constants.m_value, 9);
    }

    TEST_F(ExpressionEvaluatorTests, UnbracedNamesAreCaseSensitiveAndDeduplicated)
    {
        const auto parsed = Parse("x*x+max(y,x)+_z2+X", false);
        ASSERT_TRUE(parsed) << parsed.m_error.c_str();
        EXPECT_EQ(parsed.m_value.m_variables, (AZStd::vector<AZStd::string>{ "x", "y", "_z2", "X" }));
        const auto values = Bind(parsed.m_value, { { "X", 2 }, { "_z2", 1 }, { "y", 10 }, { "x", 3 }, { "unused", 100 } });
        ASSERT_TRUE(values) << values.m_error.c_str();
        const auto result = Evaluate(parsed.m_value, values.m_value);
        ASSERT_TRUE(result) << result.m_error.c_str();
        EXPECT_DOUBLE_EQ(result.m_value, 22);
    }

    TEST_F(ExpressionEvaluatorTests, BooleanCaseFoldingDoesNotConsumeVariableNamesOrBracedPins)
    {
        for (const auto& [spelling, expected] : { ExpressionCase{ "true", 1 }, ExpressionCase{ "false", 0 } })
        {
            for (unsigned int mask = 0; mask < (1U << AZStd::string_view(spelling).size()); ++mask)
            {
                AZStd::string source = spelling;
                for (size_t i = 0; i < source.size(); ++i)
                {
                    if (mask & (1U << i))
                    {
                        source[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(source[i])));
                    }
                }
                const auto result = EvaluateSource(source);
                ASSERT_TRUE(result) << source.c_str() << ": " << result.m_error.c_str();
                EXPECT_DOUBLE_EQ(result.m_value, expected) << source.c_str();
            }
        }
        const auto result = EvaluateSource("TrUe+FALSE+trueValue+{TRUE}", { { "trueValue", 2 }, { "TRUE", 3 } });
        ASSERT_TRUE(result) << result.m_error.c_str();
        EXPECT_DOUBLE_EQ(result.m_value, 6);
        const auto names = Parse("trueValue+FALSEhood+TRUE_1");
        ASSERT_TRUE(names) << names.m_error.c_str();
        EXPECT_EQ(names.m_value.m_variables, (AZStd::vector<AZStd::string>{ "trueValue", "FALSEhood", "TRUE_1" }));
    }

    TEST_F(ExpressionEvaluatorTests, BraceGroupingIsAvailableWhenBracedVariablesAreDisabled)
    {
        const auto parsed = Parse("[2+3]*{4-1}", false);
        ASSERT_TRUE(parsed) << parsed.m_error.c_str();
        const auto result = Evaluate(parsed.m_value, {});
        ASSERT_TRUE(result) << result.m_error.c_str();
        EXPECT_DOUBLE_EQ(result.m_value, 15);
        EXPECT_FALSE(Parse("{2+3]", false));
    }

    TEST_F(ExpressionEvaluatorTests, BindingRequiresEveryVariableAndFiniteInputs)
    {
        const auto parsed = Parse("{condition}?{a}:{b}");
        ASSERT_TRUE(parsed) << parsed.m_error.c_str();
        EXPECT_FALSE(Bind(parsed.m_value, { { "condition", 1 }, { "a", 2 } }));
        EXPECT_FALSE(Bind(parsed.m_value, { { "condition", 0 }, { "b", 3 } }));
        EXPECT_FALSE(Evaluate(parsed.m_value, { 1, 2 }));
        EXPECT_FALSE(Evaluate(parsed.m_value, { 1, 2, 3, 4 }));
        for (double invalid : { std::numeric_limits<double>::infinity(),
                                -std::numeric_limits<double>::infinity(),
                                std::numeric_limits<double>::quiet_NaN(),
                                static_cast<double>(std::numeric_limits<float>::infinity()) })
        {
            EXPECT_FALSE(Bind(parsed.m_value, { { "condition", 1 }, { "a", 2 }, { "b", invalid } }));
            EXPECT_FALSE(Evaluate(parsed.m_value, { 1, 2, invalid }));
        }
        for (const char* name : { "Cats", "nan", "NaN", "inf", "infinity" })
        {
            const auto missing = EvaluateSource(name);
            EXPECT_FALSE(missing) << name;
            EXPECT_NE(missing.m_error.find("Missing variable"), AZStd::string::npos) << name;
        }
    }

    TEST_F(ExpressionEvaluatorTests, TokenizerRejectsEmbeddedNulAndNonAsciiOutsideVariableNames)
    {
        EXPECT_FALSE(Tokenize(AZStd::string("1\0+2", 4)));
        EXPECT_FALSE(Tokenize(AZStd::string("1+2\0", 4)));
        EXPECT_FALSE(Tokenize("1\xc2\xa0+2"));
        EXPECT_FALSE(Tokenize("1\xe2\x88\x92"
                              "2"));
        const auto result = EvaluateSource("{caf\xc3\xa9}+1", { { "caf\xc3\xa9", 2 } });
        ASSERT_TRUE(result) << result.m_error.c_str();
        EXPECT_DOUBLE_EQ(result.m_value, 3);
    }

    TEST_F(ExpressionEvaluatorTests, NegativeZeroIsPreservedByLiteralsAndRemainders)
    {
        for (const char* source : { "-0.0", "-4%2", "0?-1:-0.0" })
        {
            const auto result = EvaluateSource(source);
            ASSERT_TRUE(result) << source << ": " << result.m_error.c_str();
            EXPECT_EQ(result.m_value, 0) << source;
            EXPECT_TRUE(std::signbit(result.m_value)) << source;
        }
    }

    TEST_F(ExpressionEvaluatorTests, ShuntingYardProducesRpnForLegacySimpleAndComplexTrees)
    {
        const auto simple = Parse("3+1");
        ASSERT_TRUE(simple) << simple.m_error.c_str();
        ASSERT_EQ(simple.m_value.m_instructions.size(), 3);
        EXPECT_DOUBLE_EQ(simple.m_value.m_instructions[0].m_value, 3);
        EXPECT_DOUBLE_EQ(simple.m_value.m_instructions[1].m_value, 1);
        EXPECT_EQ(simple.m_value.m_instructions[2].m_text, "+");
        const Program tree{ { { Kind::Number, {}, 2 },
                              { Kind::Number, {}, 2 },
                              { Kind::Operator, "+" },
                              { Kind::Number, {}, 4 },
                              { Kind::Number, {}, 3 },
                              { Kind::Operator, "-" },
                              { Kind::Operator, "*" },
                              { Kind::Number, {}, 2 },
                              { Kind::Operator, "/" } },
                            {} };
        const auto result = Evaluate(tree, {});
        ASSERT_TRUE(result) << result.m_error.c_str();
        EXPECT_DOUBLE_EQ(result.m_value, 2);
    }

    TEST_F(ExpressionEvaluatorTests, EvaluatorRejectsMalformedPrograms)
    {
        const Program programs[] = {
            {},
            { { { Kind::Number, {}, 1 }, { Kind::Number, {}, 2 } }, {} },
            { { { Kind::Operator, "+" } }, {} },
            { { { Kind::Number, {}, 1 }, { Kind::Operator, "+" } }, {} },
            { { { Kind::Number, {}, 1 }, { Kind::Operator, "unknown" } }, {} },
            { { { Kind::Function, "sqrt" } }, {} },
            { { { Kind::Variable, "x", 0, 1 } }, { "x" } },
            { { { Kind::Name, "x" } }, {} },
            { { { Kind::Number, {}, std::numeric_limits<double>::infinity() } }, {} },
            { { { Kind::Jump, ":", 0, 0 } }, {} },
            { { { Kind::Jump, ":", 0, 2 } }, {} },
            { { { Kind::JumpIfFalse, "?", 0, 1 } }, {} },
        };
        for (const auto& program : programs)
        {
            const auto result = Evaluate(program, AZStd::vector<double>(program.m_variables.size(), 1));
            EXPECT_FALSE(result);
            EXPECT_FALSE(result.m_error.empty());
        }
    }
} // namespace ScriptCanvasUnitTest
