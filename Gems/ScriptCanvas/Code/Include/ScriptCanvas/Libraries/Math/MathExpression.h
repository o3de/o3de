/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot&, CombinedSlotType targetSlotType, const Slot*) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }

                ExpressionEvaluation::ParseOutcome ParseExpression(const AZStd::string& formatString) override;

                AZStd::string GetExpressionSeparator() const override;
            };
        }
    }
}

//#include <Include/ScriptCanvas/Libraries/Math/MathExpression.generated.h>
