/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <gtest/gtest-param-test.h>

#include <ExpressionEvaluation/ExpressionEngine.h>
#include <Tests/ExpressionEngineTestFixture.h>

#include <ExpressionEngine/ExpressionPrimitive.h>
#include <ExpressionEngine/ExpressionVariable.h>
#include <ExpressionEngine/InternalTypes.h>

namespace ExpressionEvaluation
{
    using namespace UnitTest;     
        
    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_CoreTest_TestFixture)
    {
        ExpressionResult treeResult;
        ConfirmFailure(treeResult);

        treeResult = 2.0;
        ConfirmResult<double>(treeResult, 2.0);

        AZStd::string resultString("MyTestString");
        treeResult = resultString;

        ConfirmResult<AZStd::string>(treeResult, resultString);            
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_CoreTest_NumericPrimitiveParser)
    {
        NumericPrimitiveParser numericParser;

        {
            ExpressionElementParser::ParseResult result = numericParser.ParseElement("123", 0);

            EXPECT_EQ(result.m_charactersConsumed, 3);
            EXPECT_EQ(result.m_element.m_id, InternalTypes::Primitive);

            if (result.m_element.m_id == InternalTypes::Primitive)
            {
                EXPECT_TRUE(result.m_element.m_extraStore.is<double>());
                EXPECT_DOUBLE_EQ(Utils::GetAnyValue<double>(result.m_element.m_extraStore), 123.0);
            }
        }

        {
            ExpressionElementParser::ParseResult result = numericParser.ParseElement("0.12", 0);

            EXPECT_EQ(result.m_charactersConsumed, 4);
            EXPECT_EQ(result.m_element.m_id, InternalTypes::Primitive);

            if (result.m_element.m_id == InternalTypes::Primitive)
            {
                EXPECT_TRUE(result.m_element.m_extraStore.is<double>());
                EXPECT_DOUBLE_EQ(Utils::GetAnyValue<double>(result.m_element.m_extraStore), 0.12);
            }
        }

        {
            ExpressionElementParser::ParseResult result = numericParser.ParseElement("Cats", 0);

            EXPECT_EQ(result.m_charactersConsumed, 0);
            EXPECT_EQ(result.m_element.m_id, -1);
        }
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_CoreTest_BooleanCaseSensitiveCheck)
    {
        BooleanPrimitiveParser booleanParser;

        for (auto trueString : { "true", "tRuE", "TRUE" })
        {
            auto elementResult = booleanParser.ParseElement(trueString, 0);
                
            EXPECT_EQ(elementResult.m_charactersConsumed, 4);
            EXPECT_EQ(elementResult.m_element.m_id, InternalTypes::Primitive);

            if (elementResult.m_element.m_id == InternalTypes::Primitive)
            {
                EXPECT_TRUE(elementResult.m_element.m_extraStore.is<bool>());
                EXPECT_TRUE(Utils::GetAnyValue<bool>(elementResult.m_element.m_extraStore));
            }
        }

        for (auto trueString : { "false","FaLsE", "FALSE" })
        {
            auto elementResult = booleanParser.ParseElement(trueString, 0);

            EXPECT_EQ(elementResult.m_charactersConsumed, 5);
            EXPECT_EQ(elementResult.m_element.m_id, InternalTypes::Primitive);

            if (elementResult.m_element.m_id == InternalTypes::Primitive)
            {
                EXPECT_TRUE(elementResult.m_element.m_extraStore.is<bool>());
                EXPECT_FALSE(Utils::GetAnyValue<bool>(elementResult.m_element.m_extraStore));
            }
        }
    }

    TEST_F(ExpressionEngineTestFixture, ExpressionEngine_CoreTest_VariableParser)
    {
        VariableParser variableParser;

        ExpressionElementParser::ParseResult parseResult = variableParser.ParseElement("{Cats}*{Dogs}", 0);

        EXPECT_EQ(parseResult.m_charactersConsumed, 6);
        EXPECT_EQ(parseResult.m_element.m_id, InternalTypes::Variable);

        if (parseResult.m_element.m_id == InternalTypes::Variable)
        {
            VariableDescriptor descriptor = Utils::GetAnyValue<VariableDescriptor>(parseResult.m_element.m_extraStore);

            EXPECT_EQ(descriptor.m_displayName, AZStd::string("Cats"));
            EXPECT_EQ(descriptor.m_nameHash, AZ_CRC_CE("Cats"));
        }
    }
}
