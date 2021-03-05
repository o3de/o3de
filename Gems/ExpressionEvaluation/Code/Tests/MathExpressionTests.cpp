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

#include <AzCore/UnitTest/TestTypes.h>
#include <gtest/gtest-param-test.h>

#include <ExpressionEvaluation/ExpressionEngine.h>
#include <Tests/ExpressionEngineTestFixture.h>

#include <ExpressionEngine/MathOperators/MathExpressionOperators.h>

namespace ExpressionEvaluation
{
    using namespace UnitTest;

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_MathOperatorTest_AddOperator)
    {
        MathExpressionOperators operatorInterface;

        ElementInformation operatorInformation = MathExpressionOperators::AddOperator();

        ExpressionResultStack resultStack;

        {
            resultStack.emplace(3.0);
            resultStack.emplace(1.0);

            EXPECT_EQ(resultStack.size(), 2);

            operatorInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 1);

            auto treeResult = resultStack.PopAndReturn();
            ConfirmResult<double>(treeResult, 3.0+1.0);
        }

        {
            resultStack.emplace(1.0);
            resultStack.emplace(3.0);

            EXPECT_EQ(resultStack.size(), 2);

            operatorInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 1);

            auto treeResult = resultStack.PopAndReturn();
            ConfirmResult<double>(treeResult, 1.0+3.0);
        }
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_MathOperatorTest_SubtractOperator)
    {
        MathExpressionOperators operatorInterface;

        ElementInformation operatorInformation = MathExpressionOperators::SubtractOperator();

        ExpressionResultStack resultStack;

        {
            resultStack.emplace(3.0);
            resultStack.emplace(1.0);

            EXPECT_EQ(resultStack.size(), 2);

            operatorInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 1);

            auto treeResult = resultStack.PopAndReturn();
            ConfirmResult<double>(treeResult, 3.0-1.0);
        }

        {
            resultStack.emplace(1.0);
            resultStack.emplace(3.0);

            EXPECT_EQ(resultStack.size(), 2);

            operatorInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 1);

            auto treeResult = resultStack.PopAndReturn();
            ConfirmResult<double>(treeResult, 1.0-3.0);
        }
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_MathOperatorTest_MultiplyOperator)
    {
        MathExpressionOperators operatorInterface;

        ElementInformation operatorInformation = MathExpressionOperators::MultiplyOperator();

        ExpressionResultStack resultStack;

        {
            resultStack.emplace(2.0);
            resultStack.emplace(3.0);

            EXPECT_EQ(resultStack.size(), 2);

            operatorInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 1);

            auto treeResult = resultStack.PopAndReturn();
            ConfirmResult<double>(treeResult, 2.0*3.0);
        }

        {
            resultStack.emplace(3.0);
            resultStack.emplace(2.0);

            EXPECT_EQ(resultStack.size(), 2);

            operatorInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 1);

            auto treeResult = resultStack.PopAndReturn();
            ConfirmResult<double>(treeResult, 3.0*2.0);
        }
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_MathOperatorTest_DivideOperator)
    {
        MathExpressionOperators operatorInterface;

        ElementInformation operatorInformation = MathExpressionOperators::DivideOperator();

        ExpressionResultStack resultStack;

        {
            resultStack.emplace(4.0);
            resultStack.emplace(2.0);

            EXPECT_EQ(resultStack.size(), 2);

            operatorInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 1);

            auto treeResult = resultStack.PopAndReturn();
            ConfirmResult<double>(treeResult, 4.0/2.0);
        }

        {
            resultStack.emplace(2.0);
            resultStack.emplace(4.0);

            EXPECT_EQ(resultStack.size(), 2);

            operatorInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 1);

            auto treeResult = resultStack.PopAndReturn();
            ConfirmResult<double>(treeResult, 2.0/4.0);
        }

        {
            resultStack.emplace(2.0);
            resultStack.emplace(0.0);

            EXPECT_EQ(resultStack.size(), 2);

            operatorInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 0);
        }
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_MathOperatorTest_ModuloOperator)
    {
        MathExpressionOperators advandedMathInterface;
        
        ElementInformation operatorInformation = MathExpressionOperators::ModuloOperator();

        ExpressionResultStack resultStack;

        {
            resultStack.emplace(4.0);
            resultStack.emplace(2.0);

            EXPECT_EQ(resultStack.size(), 2);

            advandedMathInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 1);

            auto treeResult = resultStack.PopAndReturn();
            ConfirmResult<double>(treeResult, 4%2);
        }

        {
            resultStack.emplace(2.0);
            resultStack.emplace(4.0);

            EXPECT_EQ(resultStack.size(), 2);

            advandedMathInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 1);

            auto treeResult = resultStack.PopAndReturn();
            ConfirmResult<double>(treeResult, 2 % 4);
        }

        {
            resultStack.emplace(2.0);
            resultStack.emplace(0.0);

            EXPECT_EQ(resultStack.size(), 2);

            advandedMathInterface.EvaluateToken(operatorInformation, resultStack);

            EXPECT_EQ(resultStack.size(), 0);
        }
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_MathOperatorTest_BasicMathParser)
    {
        MathExpressionOperators mathParser;
            
        {
            ExpressionElementParser::ParseResult result = mathParser.ParseElement("+", 0);
            EXPECT_EQ(result.m_charactersConsumed, 1);
            EXPECT_EQ(result.m_element.m_id, MathExpressionOperators::MathOperatorId::Add);
            EXPECT_TRUE(result.m_element.m_extraStore.empty());
        }
            
        {
            ExpressionElementParser::ParseResult result = mathParser.ParseElement("-", 0);
            EXPECT_EQ(result.m_charactersConsumed, 1);
            EXPECT_EQ(result.m_element.m_id, MathExpressionOperators::MathOperatorId::Subtract);
            EXPECT_TRUE(result.m_element.m_extraStore.empty());
        }

        {
            ExpressionElementParser::ParseResult result = mathParser.ParseElement("*", 0);
            EXPECT_EQ(result.m_charactersConsumed, 1);
            EXPECT_EQ(result.m_element.m_id, MathExpressionOperators::MathOperatorId::Multiply);
            EXPECT_TRUE(result.m_element.m_extraStore.empty());
        }

        {
            ExpressionElementParser::ParseResult result = mathParser.ParseElement("/", 0);
            EXPECT_EQ(result.m_charactersConsumed, 1);
            EXPECT_EQ(result.m_element.m_id, MathExpressionOperators::MathOperatorId::Divide);
            EXPECT_TRUE(result.m_element.m_extraStore.empty());
        }

        {
            ExpressionElementParser::ParseResult result = mathParser.ParseElement("%", 0);

            EXPECT_EQ(result.m_charactersConsumed, 1);
            EXPECT_EQ(result.m_element.m_id, MathExpressionOperators::MathOperatorId::Modulo);
            EXPECT_TRUE(result.m_element.m_extraStore.empty());
        }

        {
            ExpressionElementParser::ParseResult result = mathParser.ParseElement("(", 0);

            EXPECT_EQ(result.m_charactersConsumed, 0);
        }

        {
            ExpressionElementParser::ParseResult result = mathParser.ParseElement("ABCDEFG", 0);

            EXPECT_EQ(result.m_charactersConsumed, 0);
        }
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_MathTest_SimpleTreeEvaluation)
    {
        {
            ExpressionTree tree;

            PushPrimitive(tree, 3.0);
            PushPrimitive(tree, 1.0);
            PushOperator(tree, Interfaces::MathOperators, MathExpressionOperators::AddOperator());

            auto result = ExpressionEvaluationRequests()->Evaluate(tree);

            ConfirmResult<double>(result, 3.0+1.0);
        }
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_MathTest_ComplexTreeEvaluation)
    {
        {
            ExpressionTree tree;
            PushPrimitive(tree, 2.0);
            PushPrimitive(tree, 2.0);
            PushOperator(tree, Interfaces::MathOperators, MathExpressionOperators::AddOperator());
            PushPrimitive(tree, 4.0);
            PushPrimitive(tree, 3.0);
            PushOperator(tree, Interfaces::MathOperators, MathExpressionOperators::SubtractOperator());
            PushOperator(tree, Interfaces::MathOperators, MathExpressionOperators::MultiplyOperator());
            PushPrimitive(tree, 2.0);
            PushOperator(tree, Interfaces::MathOperators, MathExpressionOperators::DivideOperator());

            auto result = ExpressionEvaluationRequests()->Evaluate(tree);

            ConfirmResult<double>(result, ((2.0 + 2.0)*(4.0 - 3.0)) / 2.0);
        }
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_MathTest_BasicParserPass)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseExpression("3+1");

        EXPECT_TRUE(treeOutcome.IsSuccess());

        const ExpressionTree& tree = treeOutcome.GetValue();

        EXPECT_EQ(tree.GetTreeSize(), 3);

        auto result = ExpressionEvaluationRequests()->Evaluate(tree);
        ConfirmResult<double>(result, 3+1);
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_MathTest_BasicParserPassMultiOperator)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseExpression("3+1*5");
        EXPECT_TRUE(treeOutcome.IsSuccess());

        const ExpressionTree& tree = treeOutcome.GetValue();

        // 3 Values + 2 Operators
        EXPECT_EQ(tree.GetTreeSize(), 3 + 2);

        auto result = ExpressionEvaluationRequests()->Evaluate(tree);
        ConfirmResult<double>(result, 3 + 1 * 5);
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_NumericParser_ComplexParserPass)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseExpression("(3 + 1)");
        EXPECT_TRUE(treeOutcome.IsSuccess());

        const ExpressionTree& tree = treeOutcome.GetValue();

        EXPECT_EQ(tree.GetTreeSize(), 3);

        auto result = ExpressionEvaluationRequests()->Evaluate(tree);
        ConfirmResult<double>(result, (3.0 + 1.0));
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_NumericParser_ComplexParserPassMultiOperator)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseExpression("((2.0 + 2.0)*(4.0 - 3.0)) / 2.0");
        EXPECT_TRUE(treeOutcome.IsSuccess());

        const ExpressionTree& tree = treeOutcome.GetValue();

        // 5 values + 4 operators
        EXPECT_EQ(tree.GetTreeSize(), 5 + 4);

        auto result = ExpressionEvaluationRequests()->Evaluate(tree);
        ConfirmResult<double>(result, ((2.0 + 2.0)*(4.0 - 3.0)) / 2.0);
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_NumericParser_ObtuseParserPassMultiOperator)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseExpression("((   ((   )  )(       2.0) + 2.0)*(4.0 - 3.0)) / (((2.0              )))");
        EXPECT_TRUE(treeOutcome.IsSuccess());

        const ExpressionTree& tree = treeOutcome.GetValue();

        // 5 values + 4 operators
        EXPECT_EQ(tree.GetTreeSize(), 5 + 4);

        auto result = ExpressionEvaluationRequests()->Evaluate(tree);
        ConfirmResult<double>(result, ((2.0 + 2.0)*(4.0 - 3.0)) / 2.0);
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_NumericParser_AdvancedParserPass)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseExpression("(3 + 1) % 2");
        EXPECT_TRUE(treeOutcome.IsSuccess());

        const ExpressionTree& tree = treeOutcome.GetValue();

        EXPECT_EQ(tree.GetTreeSize(), 3 + 2);

        auto result = ExpressionEvaluationRequests()->Evaluate(tree);
        ConfirmResult<double>(result, (3+1)%2);
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_NumericParser_BasicVariablePass)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseExpression("{A}+{B}");
        EXPECT_TRUE(treeOutcome.IsSuccess());

        ExpressionTree& tree = treeOutcome.GetValue();

        EXPECT_EQ(tree.GetTreeSize(), 2 + 1);

        auto variables = tree.GetVariables();
        EXPECT_EQ(static_cast<int>(variables.size()), 2);

        {
            AZStd::string variable = variables[0];
            EXPECT_EQ(variable, AZStd::string("A"));

            tree.SetVariable(variable, 3.0);
        }

        {
            AZStd::string variable = variables[1];
            EXPECT_EQ(variable, AZStd::string("B"));

            tree.SetVariable(variable, 1.0);
        }

        auto result = ExpressionEvaluationRequests()->Evaluate(tree);
        ConfirmResult<double>(result, 3.0+1.0);
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_NumericParser_BasicRepeatedVariablePass)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseExpression("{A}+{B}+{A}");
        EXPECT_TRUE(treeOutcome.IsSuccess());

        ExpressionTree& tree = treeOutcome.GetValue();

        EXPECT_EQ(tree.GetTreeSize(), 3 + 2);

        auto variables = tree.GetVariables();
        EXPECT_EQ(static_cast<int>(variables.size()), 2);

        {
            AZStd::string variable = variables[0];
            EXPECT_EQ(variable, AZStd::string("A"));

            tree.SetVariable(variable, 3.0);
        }

        {
            AZStd::string variable = variables[1];
            EXPECT_EQ(variable, AZStd::string("B"));

            tree.SetVariable(variable, 1.0);
        }

        auto result = ExpressionEvaluationRequests()->Evaluate(tree);
        ConfirmResult<double>(result, 3.0 + 1.0 + 3.0);
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_NumericParser_BasicFail)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseRestrictedExpression(MathOnlyOperatorRestrictions(), "true || false");
        EXPECT_FALSE(treeOutcome.IsSuccess());
        EXPECT_EQ(treeOutcome.GetError().m_offsetIndex, 0);
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_NumericParser_ComplexFail)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseRestrictedExpression(MathOnlyOperatorRestrictions(), "2+2 || 1+3");
        EXPECT_FALSE(treeOutcome.IsSuccess());
        EXPECT_EQ(treeOutcome.GetError().m_offsetIndex, 4);
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_ParenParser_UnbalancedCloseFail)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseRestrictedExpression(MathOnlyOperatorRestrictions(), "1+1)");
        EXPECT_FALSE(treeOutcome.IsSuccess());
        EXPECT_EQ(treeOutcome.GetError().m_offsetIndex, 3);
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_ParenParser_UnbalancedOpenFail)
    {
        AZ::Outcome<ExpressionTree, ParsingError> treeOutcome = ExpressionEvaluationRequests()->ParseRestrictedExpression(MathOnlyOperatorRestrictions(), "(1+1");
        EXPECT_FALSE(treeOutcome.IsSuccess());
        EXPECT_EQ(treeOutcome.GetError().m_offsetIndex, 0);
    }
}
