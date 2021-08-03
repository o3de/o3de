/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "MathExpression.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            ExpressionEvaluation::ParseOutcome MathExpression::ParseExpression(const AZStd::string& formatString)
            {
                const AZStd::unordered_set<ExpressionEvaluation::ExpressionParserId> mathInterfaceSet = {
                    ExpressionEvaluation::Interfaces::NumericPrimitives,
                    ExpressionEvaluation::Interfaces::MathOperators
                };

                ExpressionEvaluation::ParseOutcome outcome;
                ExpressionEvaluation::ExpressionEvaluationRequestBus::BroadcastResult(outcome, &ExpressionEvaluation::ExpressionEvaluationRequests::ParseRestrictedExpression, mathInterfaceSet, formatString);

                return outcome;
            }

            AZStd::string MathExpression::GetExpressionSeparator() const
            {
                return " + ";
            }
        }
    }
}
