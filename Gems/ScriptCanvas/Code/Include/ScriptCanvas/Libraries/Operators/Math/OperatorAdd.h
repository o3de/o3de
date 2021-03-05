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
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorAdd.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorAdd : public OperatorArithmetic
            {
            public:

                ScriptCanvas_Node(OperatorAdd,
                    ScriptCanvas_Node::Name("Add (+)")
                    ScriptCanvas_Node::Uuid("{C1B42FEC-0545-4511-9FAC-11E0387FEDF0}")
                    ScriptCanvas_Node::Description("Adds two or more values")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Math")
                );

                OperatorAdd() = default;

                void Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result) override;
                AZStd::unordered_set< Data::Type > GetSupportedNativeDataTypes() const override;

            protected:

                bool IsValidArithmeticSlot(const SlotId& slotId) const override;
            };
        }
    }
}