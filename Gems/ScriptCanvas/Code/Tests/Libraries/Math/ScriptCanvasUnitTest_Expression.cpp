/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Debugger/StatusBus.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionInterpretedAPI.h>
#include <ScriptCanvas/Libraries/Math/MathExpression.h>
#include <ScriptCanvas/Utils/Expression.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>

#include <AzCore/Serialization/Locale.h>
#include <cmath>

namespace ScriptCanvasUnitTest
{
    AZ_CVAR(double, sc_eval_test_value, 9, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Expression console test value");

    class ExpressionConsoleOutput : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        ExpressionConsoleOutput()
        {
            BusConnect();
        }
        ~ExpressionConsoleOutput() override
        {
            BusDisconnect();
        }
        bool OnPrintf(const char* window, const char* message) override
        {
            if (AZStd::string_view(window) == "ScriptCanvas")
            {
                m_output += message;
                return true;
            }
            return false;
        }
        AZStd::string m_output;
    };

    class ScriptCanvasMathExpressionTests : public ScriptCanvasUnitTestFixture
    {
    protected:
        static ScriptCanvas::Expression::Result<ScriptCanvas::Expression::Program> Parse(const AZStd::string& source)
        {
            const auto tokens = ScriptCanvas::Expression::Tokenize(source, true);
            if (!tokens)
            {
                return { {}, tokens.m_error };
            }
            return ScriptCanvas::Expression::ShuntingYard(tokens.m_value);
        }

        void CheckLua(const AZStd::string& source, const ScriptCanvas::Expression::Bindings& bindings = {})
        {
            SCOPED_TRACE(source.c_str());
            const auto parsed = Parse(source);
            ASSERT_TRUE(parsed) << parsed.m_error.c_str();
            const auto values = ScriptCanvas::Expression::Bind(parsed.m_value, bindings);
            ASSERT_TRUE(values) << values.m_error.c_str();
            const auto expected = ScriptCanvas::Expression::Evaluate(parsed.m_value, values.m_value);
            auto lua = ScriptCanvas::Expression::ToLua(parsed.m_value);
            ASSERT_TRUE(lua) << lua.m_error.c_str();
            AZ::Locale::ScopedSerializationLocale locale;
            for (double value : values.m_value)
            {
                const auto at = lua.m_value.find('@');
                ASSERT_NE(at, AZStd::string::npos);
                lua.m_value.replace(at, 1, AZStd::string::format("%.17g", value));
            }
            lua_State* state = luaL_newstate();
            ASSERT_NE(state, nullptr);
            lua_register(state, ScriptCanvas::Expression::LuaOperationName, &ScriptCanvas::Execution::MathExpressionOperation);
            const auto script = "return " + lua.m_value;
            const int loaded = luaL_loadbuffer(state, script.data(), script.size(), "MathExpressionTest");
            EXPECT_EQ(loaded, LUA_OK) << (loaded == LUA_OK ? "" : lua_tostring(state, -1));
            if (loaded == LUA_OK)
            {
                const int status = lua_pcall(state, 0, 1, 0);
                if (expected)
                {
                    EXPECT_EQ(status, LUA_OK) << (status == LUA_OK ? "" : lua_tostring(state, -1));
                    EXPECT_EQ(lua_type(state, -1), LUA_TNUMBER);
                    EXPECT_NEAR(lua_tonumber(state, -1), expected.m_value, 1e-12 * std::fmax(1, std::abs(expected.m_value)));
                }
                else
                {
                    EXPECT_NE(status, LUA_OK);
                }
            }
            lua_close(state);
        }
    };

    TEST_F(ScriptCanvasMathExpressionTests, LuaMatchesCppOperatorsFunctionsAndLazyTernaries)
    {
        for (const char* expression : { "2 + 3 * 4 - 5 / 2",
                                        "-5 % 2",
                                        "5.5 % 2",
                                        "2^3^2",
                                        "2**3**2",
                                        "-2^2",
                                        "2^-2",
                                        "1<2",
                                        "2<=2",
                                        "3>2",
                                        "3>=3",
                                        "1==2",
                                        "2!=3",
                                        "!0",
                                        "!!5",
                                        "0&&2",
                                        "1||0",
                                        "(1<2) + (2>3)",
                                        "false || true && false",
                                        "sin(pi/2)+cos(0)+tan(0)",
                                        "asin(1)+acos(0)-atan(1)*4",
                                        "atan2(1,0)",
                                        "sinh(0)+cosh(0)+tanh(0)",
                                        "asinh(0)+acosh(1)+atanh(0)",
                                        "exp(0)+exp2(3)+sqrt(9)+cbrt(-8)",
                                        "log(e)+ln(e)+log2(8)+log10(100)",
                                        "abs(-2)+floor(1.9)+ceil(1.1)+round(-1.5)+trunc(-1.9)",
                                        "max(1,min(3,pow(2,3)))+hypot(3,4)",
                                        "1?7:1/0",
                                        "0?sqrt(-1):8",
                                        "0?2:0?4:5",
                                        "1?0?1/0:6:sqrt(-1)",
                                        "(1?0:1)?2:3",
                                        "max(1?2:3,0?4:5)",
                                        "1?0:9",
                                        "0||1?2:3",
                                        "0&&1/0",
                                        "1||sqrt(-1)",
                                        "1/0",
                                        "exp(1000)" })
        {
            CheckLua(expression);
        }

        CheckLua("{x}>0 ? sqrt({x}) : {fallback}", { { "x", -4 }, { "fallback", 7 } });
        CheckLua("{x}>0 ? sqrt({x}) : {fallback}", { { "x", 9 }, { "fallback", 7 } });
        CheckLua("{pi}+{input with spaces}*{pi}", { { "pi", 2 }, { "input with spaces", 3 } });
    }

    TEST_F(ScriptCanvasMathExpressionTests, ConsoleFunctorBindsCvarsAndReportsErrors)
    {
        AZ::Console console;
        console.LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
        AZ::Interface<AZ::IConsole>::Register(&console);
        ExpressionConsoleOutput capture;
        EXPECT_NE(console.FindCommand("sc_eval"), nullptr);
        EXPECT_TRUE(console.PerformCommand("sc_eval_test_value 9"));
        EXPECT_TRUE(console.PerformCommand("sc_eval {sc_eval_test_value} > 0 ? sqrt(sc_eval_test_value) : 1/0"));
        EXPECT_NE(capture.m_output.find("sc_eval: 3\n"), AZStd::string::npos);
        capture.m_output.clear();
        EXPECT_TRUE(console.PerformCommand("sc_eval_test_value -4"));
        EXPECT_TRUE(console.PerformCommand("sc_eval sc_eval_test_value > 0 ? sqrt(sc_eval_test_value) : 7"));
        EXPECT_NE(capture.m_output.find("sc_eval: 7\n"), AZStd::string::npos);
        capture.m_output.clear();
        EXPECT_TRUE(console.PerformCommand("sc_eval \"sin(pi / 2)\""));
        EXPECT_NE(capture.m_output.find("sc_eval: 1\n"), AZStd::string::npos);
        capture.m_output.clear();
        EXPECT_TRUE(console.PerformCommand("sc_eval missing_expression_test_cvar + 1"));
        EXPECT_NE(capture.m_output.find("Unknown or non-numeric console variable"), AZStd::string::npos);
        capture.m_output.clear();
        EXPECT_TRUE(console.PerformCommand("sc_eval 1 / 0"));
        EXPECT_NE(capture.m_output.find("Division by zero"), AZStd::string::npos);
        capture.m_output.clear();
        EXPECT_TRUE(console.PerformCommand("sc_eval 1 +"));
        EXPECT_NE(capture.m_output.find("incomplete expression"), AZStd::string::npos);
        capture.m_output.clear();
        EXPECT_TRUE(console.PerformCommand("sc_eval"));
        EXPECT_NE(capture.m_output.find("Usage: sc_eval"), AZStd::string::npos);
        AZ::Interface<AZ::IConsole>::Unregister(&console);
    }

    class EditableMathExpression : public ScriptCanvas::Nodes::Math::MathExpression
    {
    public:
        void SetExpression(const AZStd::string& source)
        {
            m_format = source;
            ParseFormat(false);
        }
        void Restore()
        {
            OnInit();
        }
        ScriptCanvas::SlotId AddInput()
        {
            return HandleExtension(GetExtensionId());
        }
        void FinishInput()
        {
            FinalizeExtension(GetExtensionId());
        }
        bool IsValid()
        {
            ScriptCanvas::ValidationResults results;
            return OnValidateNode(results);
        }
    };

    TEST_F(ScriptCanvasMathExpressionTests, ExpressionEditsPreservePinIdsDefaultsAndRepeatedVariables)
    {
        AZ::Entity entity("Math Expression");
        auto* node = aznew EditableMathExpression;
        entity.AddComponent(node);
        node->ConfigureSlots();
        node->SetExpression("{x}+{y}+{x}");
        const auto x = node->GetSlotId("x");
        const auto y = node->GetSlotId("y");
        ASSERT_TRUE(x.IsValid());
        ASSERT_TRUE(y.IsValid());
        EXPECT_EQ(node->GetSlotsByName().size(), 2);
        EXPECT_EQ(node->FindDatum(x)->GetType(), ScriptCanvas::Data::Type::Number());
        {
            ScriptCanvas::ModifiableDatumView datum;
            ASSERT_TRUE(node->FindModifiableDatumView(x, datum));
            datum.SetAs(17.0);
        }
        node->SetExpression("{y}>0 ? {x}^2 : {new pin}");
        EXPECT_EQ(node->GetSlotId("x"), x);
        EXPECT_EQ(node->GetSlotId("y"), y);
        ASSERT_NE(node->FindDatum(x), nullptr);
        EXPECT_DOUBLE_EQ(*node->FindDatum(x)->GetAs<double>(), 17);
        EXPECT_EQ(node->GetSlotsByName().size(), 3);
        node->Restore();
        EXPECT_EQ(node->GetSlotId("x"), x);
        EXPECT_EQ(node->GetSlotsByName().at("x"), x);
        node->SetExpression("{x} ?");
        EXPECT_FALSE(node->IsValid());
        EXPECT_EQ(node->GetSlotId("x"), x);
        EXPECT_EQ(node->GetSlotId("y"), y);
        node->SetExpression("{x}");
        EXPECT_TRUE(node->IsValid());
        EXPECT_EQ(node->GetSlotId("x"), x);
        EXPECT_FALSE(node->GetSlotId("y").IsValid());
        EXPECT_EQ(node->GetSlotsByName().size(), 1);
        node->SetExpression("{Result}+1");
        EXPECT_FALSE(node->IsValid());
        EXPECT_EQ(node->GetSlotId("x"), x);
    }

    TEST_F(ScriptCanvasMathExpressionTests, EmptyNodeCanAddItsFirstVariablePin)
    {
        AZ::Entity entity("Math Expression");
        auto* node = aznew EditableMathExpression;
        entity.AddComponent(node);
        node->ConfigureSlots();
        node->Restore();
        const auto input = node->AddInput();
        EXPECT_TRUE(input.IsValid());
        EXPECT_EQ(node->GetSlotId("Value"), input);
        EXPECT_EQ(node->GetRawFormat(), "{Value}");
        EXPECT_TRUE(node->IsValid());
        node->FinishInput();
    }

    TEST_F(ScriptCanvasMathExpressionTests, VersionConversionDiscardsOnlyTheOldParserCache)
    {
        AZ::SerializeContext context;
        AZ::SerializeContext::DataElementNode element;
        ASSERT_GE(element.AddElementWithData(context, "m_format", AZStd::string("{x}+1")), 0);
        ASSERT_GE(element.AddElementWithData(context, "m_expressionTree", 42), 0);
        EXPECT_TRUE(ScriptCanvas::Nodes::Internal::ExpressionNodeBase::VersionConverter(context, element));
        EXPECT_EQ(element.FindElement(AZ_CRC_CE("m_expressionTree")), -1);
        const int format = element.FindElement(AZ_CRC_CE("m_format"));
        ASSERT_GE(format, 0);
        AZStd::string text;
        ASSERT_TRUE(element.GetSubElement(format).GetData(text));
        EXPECT_EQ(text, "{x}+1");
    }
} // namespace ScriptCanvasUnitTest
