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

#include <ScriptCanvas/CodeGen/CodeGen.h>
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
            class MathExpression
                : public Internal::ExpressionNodeBase
            {        
                ScriptCanvas_Node(MathExpression,
                    ScriptCanvas_Node::Name("Math Expression", "Will evaluate a series of math operations, allowing users to specify inputs using {}.")
                    ScriptCanvas_Node::Uuid("{A5841DE8-CA11-4364-9C34-5ECE8B9623D7}")
                    ScriptCanvas_Node::Category("Math")
                    ScriptCanvas_Node::EditAttributes(AZ::Edit::Attributes::CategoryStyle(".math"), ScriptCanvas::Attributes::Node::TitlePaletteOverride("MathNodeTitlePalette"))
                    ScriptCanvas_Node::DynamicSlotOrdering(true)
                    ScriptCanvas_Node::Version(0)
                );
            public:

                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Output signal"));

                ScriptCanvas_Property(double,
                    ScriptCanvas_Property::Name("Result", "The resulting string.")
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::DisplayGroup("ExpressionDisplayGroup")
                );

            protected:

                void OnResult(const ExpressionEvaluation::ExpressionResult& result) override;
                ExpressionEvaluation::ParseOutcome ParseExpression(const AZStd::string& formatString) override;

                AZStd::string GetExpressionSeparator() const override;
            };
        }
    }
}

