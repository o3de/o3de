/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Math/MathUtils.h>

#include <ExpressionEngine/MathOperators/MathExpressionOperators.h>
#include <ExpressionEngine/Utils.h>

#include <ExpressionEvaluation/ExpressionEngine/ExpressionTypes.h>


namespace ExpressionEvaluation
{
    ////////////////////////////
    // MathExpressionOperators
    ////////////////////////////

    ElementInformation MathExpressionOperators::AddOperator()
    {
        ElementInformation elementInfo;

        elementInfo.m_id = MathExpressionOperators::Add;
        elementInfo.m_priority = AddSubtract;

        return elementInfo;
    }

    ElementInformation MathExpressionOperators::SubtractOperator()
    {
        ElementInformation elementInfo;

        elementInfo.m_id = MathExpressionOperators::Subtract;
        elementInfo.m_priority = AddSubtract;

        return elementInfo;
    }

    ElementInformation MathExpressionOperators::MultiplyOperator()
    {
        ElementInformation elementInfo;

        elementInfo.m_id = MathExpressionOperators::Multiply;
        elementInfo.m_priority = MultiplyDivideModulo;

        return elementInfo;
    }

    ElementInformation MathExpressionOperators::DivideOperator()
    {
        ElementInformation elementInfo;

        elementInfo.m_id = MathExpressionOperators::Divide;
        elementInfo.m_priority = MultiplyDivideModulo;

        return elementInfo;
    }

    ElementInformation MathExpressionOperators::ModuloOperator()
    {
        ElementInformation elementInfo;

        elementInfo.m_id = MathExpressionOperators::Modulo;
        elementInfo.m_priority = MultiplyDivideModulo;

        return elementInfo;
    }

    ExpressionParserId MathExpressionOperators::GetParserId() const
    {
        return Interfaces::MathOperators;
    }

    MathExpressionOperators::ParseResult MathExpressionOperators::ParseElement(const AZStd::string& inputText, size_t offset) const
    {
        ParseResult result;
            
        char firstChar = inputText.at(offset);
            
        if (firstChar == '+')
        {
            result.m_charactersConsumed = 1;
            
            result.m_element = AddOperator();
        }
        else if (firstChar == '-')
        {
            result.m_charactersConsumed = 1;
            
            result.m_element = SubtractOperator();
        }
        else if (firstChar == '*')
        {
            result.m_charactersConsumed = 1;
            
            result.m_element = MultiplyOperator();
        }
        else if (firstChar == '/')
        {
            result.m_charactersConsumed = 1;
            
            result.m_element = DivideOperator();
        }
        else if (firstChar == '%')
        {
            result.m_charactersConsumed = 1;

            result.m_element = ModuloOperator();
        }

        return result;
    }

    void MathExpressionOperators::EvaluateToken(const ElementInformation& elementInformation, ExpressionResultStack& resultStack) const
    {
        if (resultStack.size() < 2)
        {
            return;
        }

        auto rightValue = resultStack.PopAndReturn();
        auto leftValue = resultStack.PopAndReturn();

        ExpressionResult result;

        switch (elementInformation.m_id)
        {
        case Add:
            result = OnAddOperator(leftValue, rightValue);
            break;
        case Subtract:
            result = OnSubtractOperator(leftValue, rightValue);
            break;
        case Multiply:
            result = OnMultiplyOperator(leftValue, rightValue);
            break;
        case Divide:
            result = OnDivideOperator(leftValue, rightValue);
            break;
        case Modulo:
            result = OnModuloOperator(leftValue, rightValue);
            break;
        default:
            break;
        }

        if (!result.empty())
        {
            resultStack.emplace(AZStd::move(result));
        }
    }

    ExpressionResult MathExpressionOperators::OnAddOperator(const AZStd::any& leftValue, const AZStd::any& rightValue) const
    {
        double lhsValue = Utils::GetAnyValue<double>(leftValue);
        double rhsValue = Utils::GetAnyValue<double>(rightValue);

        return ExpressionResult(lhsValue + rhsValue);
    }

    ExpressionResult MathExpressionOperators::OnSubtractOperator(const AZStd::any& leftValue, const AZStd::any& rightValue) const
    {
        double lhsValue = Utils::GetAnyValue<double>(leftValue);
        double rhsValue = Utils::GetAnyValue<double>(rightValue);

        return ExpressionResult(lhsValue - rhsValue);
    }

    ExpressionResult MathExpressionOperators::OnMultiplyOperator(const AZStd::any& leftValue, const AZStd::any& rightValue) const
    {
        double lhsValue = Utils::GetAnyValue<double>(leftValue);
        double rhsValue = Utils::GetAnyValue<double>(rightValue);

        return ExpressionResult(lhsValue * rhsValue);
    }

    ExpressionResult MathExpressionOperators::OnDivideOperator(const AZStd::any& leftValue, const AZStd::any& rightValue) const
    {
        double lhsValue = Utils::GetAnyValue<double>(leftValue);
        double rhsValue = Utils::GetAnyValue<double>(rightValue);

        if (!AZ::IsClose(rhsValue, 0.0, std::numeric_limits<double>::epsilon()))
        {
            return ExpressionResult(lhsValue / rhsValue);
        }

        return ExpressionResult();
    }

    ExpressionResult MathExpressionOperators::OnModuloOperator(const AZStd::any& leftValue, const AZStd::any& rightValue) const
    {
        double lhsValue = Utils::GetAnyValue<double>(leftValue);
        double rhsValue = Utils::GetAnyValue<double>(rightValue);

        if (!AZ::IsClose(rhsValue, 0.0, std::numeric_limits<double>().epsilon()))
        {
            return ExpressionResult(aznumeric_cast<double>(aznumeric_cast<int>(lhsValue) % aznumeric_cast<int>(rhsValue)));
        }

        return ExpressionResult();
    }
}
