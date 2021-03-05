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
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorMul.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorMul : public OperatorArithmetic
            {
            public:

                ScriptCanvas_Node(OperatorMul,
                    ScriptCanvas_Node::Name("Multiply (*)")
                    ScriptCanvas_Node::Uuid("{E9BB45A1-AE96-47B0-B2BF-2927D420A28C}")
                    ScriptCanvas_Node::Description("Multiplies two of more values")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Math")
                );

                OperatorMul() = default;

                AZStd::string_view OperatorFunction() const override { return "Multiply"; }

                AZStd::unordered_set< Data::Type > GetSupportedNativeDataTypes() const override;
                void Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result) override;
   
            protected:

                void InitializeSlot(const SlotId& slotId, const ScriptCanvas::Data::Type& dataType) override;
                bool IsValidArithmeticSlot(const SlotId& slotId) const override;

                void OnResetDatumToDefaultValue(ModifiableDatumView& datum) override;
            };
        }
    }
}