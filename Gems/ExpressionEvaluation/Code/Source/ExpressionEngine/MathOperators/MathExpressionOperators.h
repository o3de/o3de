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
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <ExpressionEngine/ExpressionElementParser.h>

namespace ExpressionEvaluation
{
    class MathExpressionOperators
        : public ExpressionElementParser
    {
    public:
        AZ_CLASS_ALLOCATOR(MathExpressionOperators, AZ::SystemAllocator, 0);

        enum MathExpressionOperatorPriority
        {
            Unknown = -1,
            AddSubtract = 0,
            MultiplyDivideModulo = 1,
            Power = 2,
            Function = 3
        };

        enum MathOperatorId
        {
            Add = 0,
            Subtract,
            Multiply,
            Divide,
            Modulo
        };

        static ElementInformation AddOperator();
        static ElementInformation SubtractOperator();
        static ElementInformation MultiplyOperator();
        static ElementInformation DivideOperator();
        static ElementInformation ModuloOperator();

        MathExpressionOperators() = default;
        ~MathExpressionOperators() override = default;

        ExpressionParserId GetParserId() const override;

        ParseResult ParseElement(const AZStd::string& inputText, size_t offset) const override;
        void EvaluateToken(const ElementInformation& elementInformation, ExpressionResultStack& evaluationStack) const override;

    private:

        ExpressionResult OnAddOperator(const AZStd::any& leftValue, const AZStd::any& rightValue) const;
        ExpressionResult OnSubtractOperator(const AZStd::any& leftValue, const AZStd::any& rightValue) const;
        ExpressionResult OnMultiplyOperator(const AZStd::any& leftValue, const AZStd::any& rightValue) const;
        ExpressionResult OnDivideOperator(const AZStd::any& leftValue, const AZStd::any& rightValue) const;
        ExpressionResult OnModuloOperator(const AZStd::any& leftValue, const AZStd::any& rightValue) const;
    };
}
