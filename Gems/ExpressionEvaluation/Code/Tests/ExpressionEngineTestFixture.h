/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <gtest/gtest-param-test.h>

#include <ExpressionEvaluation/ExpressionEngine.h>
#include <ExpressionEvaluationSystemComponent.h>
#include <ExpressionEngine/Utils.h>

namespace ExpressionEvaluation
{
    class ExpressionEngineTestFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:

        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            // Faking the setup to avoid needing to re-implement these features somewhere else.
            m_systemComponent = aznew ExpressionEvaluationSystemComponent();
            m_systemComponent->Init();
            m_systemComponent->Activate();
        }

        void TearDown() override
        {
            m_systemComponent->Deactivate();
            delete m_systemComponent;

            UnitTest::LeakDetectionFixture::TearDown();
        }

        template<typename T>
        void ConfirmResult(const ExpressionResult& result, const T& knownValue)
        {
            EXPECT_FALSE(result.type().IsNull());
            EXPECT_EQ(result.type(), azrtti_typeid<T>());

            T resultValue = Utils::GetAnyValue<T>(result);
            EXPECT_EQ(resultValue, knownValue);
        }

        void ConfirmFailure(const ExpressionResult& result)
        {
            EXPECT_TRUE(result.type().IsNull());
        }

        template<typename T>
        void PushPrimitive(ExpressionTree& expressionTree, const T& primitiveValue)
        {
            ExpressionToken token;

            if (AZStd::is_same<T, double>::value)
            {
                token.m_parserId = Interfaces::NumericPrimitives;
            }
            else if (AZStd::is_same<T, bool>())
            {
                token.m_parserId = Interfaces::BooleanPrimitives;
            }

            token.m_information = Primitive::GetPrimitiveElement(primitiveValue);

            expressionTree.PushElement(AZStd::move(token));
        }

        void PushOperator(ExpressionTree& expressionTree, ExpressionParserId parserId, ElementInformation&& elementInformation)
        {
            ExpressionToken expressionToken;

            expressionToken.m_parserId = parserId;
            expressionToken.m_information = AZStd::move(elementInformation);

            expressionTree.PushElement(AZStd::move(expressionToken));
        }

        ExpressionEvaluationRequests* GetExpressionEvaluationRequests()
        {
            return m_systemComponent;
        }

        AZStd::unordered_set<ExpressionParserId> MathOnlyOperatorRestrictions() const
        {
            return {
                Interfaces::NumericPrimitives,
                Interfaces::MathOperators
            };
        }

    private:

        ExpressionEvaluationSystemComponent* m_systemComponent;
    };
}
