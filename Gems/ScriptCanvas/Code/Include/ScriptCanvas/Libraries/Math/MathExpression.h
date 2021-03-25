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

#include <ScriptCanvas/Core/Node.h>

#include <ScriptCanvas/Internal/Nodes/ExpressionNodeBase.h>

#include <Include/ScriptCanvas/Libraries/Math/MathExpression.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Internal
        {
            class ExpressionNodeBase;
        }

        namespace Math
        {
            //! Provides a node that can take in a mathematical expression and convert it into a single node.
            class MathExpression
                : public Internal::ExpressionNodeBase
            {

                SCRIPTCANVAS_NODE(MathExpression);

            protected:

                SlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot&, CombinedSlotType targetSlotType, const Slot*) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }

                void OnResult(const ExpressionEvaluation::ExpressionResult& result) override;
                ExpressionEvaluation::ParseOutcome ParseExpression(const AZStd::string& formatString) override;

                AZStd::string GetExpressionSeparator() const override;
            };
        }
    }
}

