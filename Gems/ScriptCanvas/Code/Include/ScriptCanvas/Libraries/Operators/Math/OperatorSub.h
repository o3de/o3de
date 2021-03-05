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

#include "OperatorArithmetic.h"
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorSub.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorSub: public OperatorArithmetic
            {
            public:

                ScriptCanvas_Node(OperatorSub,
                    ScriptCanvas_Node::Name("Subtract (-)")
                    ScriptCanvas_Node::Uuid("{D0615D0A-027F-47F6-A02B-E35DAF22F431}")
                    ScriptCanvas_Node::Description("Subtracts two of more elements")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Math")
                );

                OperatorSub() = default;

                void Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result) override;
            };
        }
    }
}