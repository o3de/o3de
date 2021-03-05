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
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorDiv.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorDiv : public OperatorArithmetic
            {
            public:

                ScriptCanvas_Node(OperatorDiv,
                    ScriptCanvas_Node::Name("Divide (/)")
                    ScriptCanvas_Node::Uuid("{DC17E19F-3829-410D-9A0B-AD60C6066DAA}")
                    ScriptCanvas_Node::Description("Divides two or more values")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Math")
                );

                OperatorDiv() = default;
                ~OperatorDiv() = default;

                AZStd::string_view OperatorFunction() const override { return "Divide"; }
                AZStd::unordered_set< Data::Type > GetSupportedNativeDataTypes() const override;

                void Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result) override;

            protected:

                void InitializeSlot(const SlotId& slotId, const ScriptCanvas::Data::Type& dataType) override;                
                bool IsValidArithmeticSlot(const SlotId& slotId) const override;

                void OnResetDatumToDefaultValue(ModifiableDatumView& datumView) override;
            };
        }
    }
}