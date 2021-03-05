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

#include <AzCore/Component/Component.h>

#include <ExpressionEvaluation/ExpressionEvaluationBus.h>

#include <ExpressionEngine/ExpressionPrimitive.h>
#include <ExpressionEngine/ExpressionVariable.h>

namespace ExpressionEvaluation
{
    class ExpressionEvaluationSystemComponent
        : public AZ::Component
        , public ExpressionEvaluationRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ExpressionEvaluationSystemComponent, "{55C70DBA-9B11-4A23-83C5-CA90260C917A}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ~ExpressionEvaluationSystemComponent();
        
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        void RegisterExpressionInterface(ExpressionElementParser* elementInterface);
        void RemoveExpressionInterface(ExpressionParserId interfaceId);
        
        // ExpressionEvaluationRequestBus
        ParseOutcome ParseExpression(AZStd::string_view expressionString) const override;
        ParseInPlaceOutcome ParseExpressionInPlace(AZStd::string_view expressionString, ExpressionTree& expressionTree) const override;

        ParseOutcome ParseRestrictedExpression(const AZStd::unordered_set<ExpressionParserId>& availableParsers, AZStd::string_view expressionString) const override;
        ParseInPlaceOutcome ParseRestrictedExpressionInPlace(const AZStd::unordered_set<ExpressionParserId>& availableParsers, AZStd::string_view expressionString, ExpressionTree& expressionTree) const override;

        EvaluateStringOutcome EvaluateExpression(AZStd::string_view expression) const override;
        ExpressionResult Evaluate(const ExpressionTree& expressionTree) const override;
        ////
        
    private:

        AZ::Outcome<void, ParsingError> ReportMissingValue(size_t offset) const;
        AZ::Outcome<void, ParsingError> ReportUnexpectedOperator(const AZStd::string& parseString, size_t offset, size_t charactersConsumed) const;
        AZ::Outcome<void, ParsingError> ReportUnexpectedValue(const AZStd::string& parseString, size_t offset, size_t charactersConsumed) const;
        AZ::Outcome<void, ParsingError> ReportUnexpectedSymbol(const AZStd::string& parseString, size_t offset, size_t charactersConsumed) const;
        AZ::Outcome<void, ParsingError> ReportUnknownCharacter(const AZStd::string& parseString, size_t offset) const;
        AZ::Outcome<void, ParsingError> ReportUnbalancedParen(size_t offset, const AZStd::string& offsetsString) const;

        AZStd::vector<ExpressionElementParser*> m_internalParsers;
    
        AZStd::unordered_map<ExpressionParserId, ExpressionElementParser*> m_elementInterfaces;
    };
}
