/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string_view.h>

#include <ExpressionEvaluation/ExpressionEngine/ExpressionTypes.h>
#include <ExpressionEvaluation/ExpressionEngine/ExpressionTree.h>

namespace ExpressionEvaluation
{
    using ParseOutcome = AZ::Outcome<ExpressionTree, ParsingError>;
    using ParseInPlaceOutcome = AZ::Outcome<void, ParsingError>;

    using EvaluateStringOutcome = AZ::Outcome<ExpressionResult, ParsingError>;

    class ExpressionEvaluationRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Parses the expression into the returned ExpressionTree.
        //! /param expressionString
        //!    The string to parse.
        //! /return Returns an Expression Tree, or a Parsing Error.
        virtual ParseOutcome ParseExpression(AZStd::string_view expressionString) const = 0;

        //! Parses the expression into the supplied ExpressionTree.
        //! /param expressionString
        //!    The string to parse.
        //! /param expressionTree
        //!     The ExpressionTree that will contain the parsed tree on Success.
        //! /return Returns Success or a Parsing Error.
        virtual ParseInPlaceOutcome ParseExpressionInPlace(AZStd::string_view expressionString, ExpressionTree& expressionTree) const = 0;
        
        //! Parses the expression into the returned ExpressionTree using the specified list of parsers.
        //! /param availableParsers
        //!    The list of parsers to use when parsing the expression.
        //! /param expressionString
        //!    The string to parse.
        //! /return Returns an Expression Tree, or a Parsing Error.
        virtual ParseOutcome ParseRestrictedExpression(const AZStd::unordered_set<ExpressionParserId>& availableParsers, AZStd::string_view expressionString) const = 0;

        //! Parses the expression into the supplied ExpressionTree using the specified list of parsers.
        //! /param availableParsers
        //!    The list of parsers to use when parsing the expression.
        //! /param expressionString
        //!    The string to parse.
        //! /param expressionTree
        //!     The ExpressionTree that will contain the parsed tree on Success.
        //! /return Returns Success, or a Parsing Error.
        virtual ParseInPlaceOutcome ParseRestrictedExpressionInPlace(const AZStd::unordered_set<ExpressionParserId>& availableParsers, AZStd::string_view expressionString, ExpressionTree& expressionTree) const = 0;

        //! Parses then Evaluates the specified Expression, and returns the result or parse error.
        //! /param expressionString
        //!    The string to parse/evaluate
        //! /return Returns the result of the expression evaluation, or a Parsing Error.
        virtual EvaluateStringOutcome EvaluateExpression(AZStd::string_view expression) const = 0;

        //! Evalutes the specified ExpressionTree
        //! /param expressionTree
        //!    The ExpressionTree to be evaluated
        //! /return Returns the result of the expression evaluation.
        virtual ExpressionResult Evaluate(const ExpressionTree& expressionTree) const = 0;
    };
    
    using ExpressionEvaluationRequestBus = AZ::EBus<ExpressionEvaluationRequests>;

} // namespace ExpressionEvaluationGem
