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
#include "OperatorArithmetic.h"
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorDivideByNumber.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
#define DIVIDABLE_TYPES { Data::Type::Number(), Data::Type::Vector2(), Data::Type::Vector3(), Data::Type::Vector4() }
            class OperatorDivideByNumber 
                : public Node
            {                
            public:
                ScriptCanvas_Node(OperatorDivideByNumber,
                    ScriptCanvas_Node::Name("Divide by Number (/)")
                    ScriptCanvas_Node::Uuid("{8305B5C9-1B9F-4D5B-B3E7-66925F491E9D}")
                    ScriptCanvas_Node::Description("Divides certain types by a given number")
                    ScriptCanvas_Node::Version(1, OperatorDivideByNumberVersionConverter)
                    ScriptCanvas_Node::Category("Math")
                );

                enum Version
                {
                    InitialVersion = 0,
                    RemoveOperatorBase,

                    Current
                };

                static bool OperatorDivideByNumberVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                OperatorDivideByNumber() = default;

                // Nodes
                void OnInit() override;
                void OnInputSignal(const SlotId& slotId) override;
                ////

                ScriptCanvas_In(ScriptCanvas_In::Name("In", ""));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

            protected:

                AZ::Crc32 GetDynamicGroupId() const { return AZ_CRC("DivideGroup", 0x66473fe4); }

                ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Value,
                    ScriptCanvas::ConnectionType::Input,
                    ScriptCanvas_DynamicDataSlot::Name("Source", "The value to be divided by a number")
                    ScriptCanvas_DynamicDataSlot::DynamicGroup("DivideGroup")
                    ScriptCanvas_DynamicDataSlot::RestrictedTypeContractTag(DIVIDABLE_TYPES)
                );

                ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Value,
                    ScriptCanvas::ConnectionType::Output,
                    ScriptCanvas_DynamicDataSlot::Name("Result", "The result of the operation")
                    ScriptCanvas_DynamicDataSlot::DynamicGroup("DivideGroup")
                    ScriptCanvas_DynamicDataSlot::RestrictedTypeContractTag(DIVIDABLE_TYPES)
                );

                ScriptCanvas_PropertyWithDefaults(ScriptCanvas::Data::NumberType, 1.f,
                    ScriptCanvas_Property::Name("Divisor", "The number to divide by.")
                    ScriptCanvas_Property::Input);

                SlotId m_operandId;
            };
        }
#undef DIVIDABLE_TYPES
    }
}