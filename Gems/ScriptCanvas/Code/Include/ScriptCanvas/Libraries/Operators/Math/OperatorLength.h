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
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorLength.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorLength : public Node
            {
            public:
                ScriptCanvas_Node(OperatorLength,
                    ScriptCanvas_Node::Name("Length")
                    ScriptCanvas_Node::Uuid("{AEE15BEA-CD51-4C1A-B06D-C09FB9EAA005}")
                    ScriptCanvas_Node::Description("Given a vector this returns the magnitude (length) of the vector. For a quaternion, magnitude is the cosine of half the angle of rotation.")
                    ScriptCanvas_Node::Version(1, OperatorLengthConverter)
                    ScriptCanvas_Node::Category("Math")
                );

                enum Version
                {
                    InitialVersion = 0,
                    RemoveOperatorBase,

                    Current
                };

                static bool OperatorLengthConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                OperatorLength() = default;
                ~OperatorLength() = default;

                // Node
                void OnInit() override;
                void OnInputSignal(const SlotId& slotId) override;
                ////

                ScriptCanvas_In(ScriptCanvas_In::Name("In", ""));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

                ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Value,
                    ScriptCanvas::ConnectionType::Input,
                    ScriptCanvas_DynamicDataSlot::Name("Source", "A vector or quaternion")
                    ScriptCanvas_DynamicDataSlot::DynamicGroup("SourceGroup")
                    ScriptCanvas_DynamicDataSlot::RestrictedTypeContractTag({ Data::Type::Vector2(), Data::Type::Vector3(), Data::Type::Vector4(), Data::Type::Quaternion() })
                    ScriptCanvas_DynamicDataSlot::SupportsMethodContractTag("Length")
                );

                ScriptCanvas_Property(int,
                    ScriptCanvas_Property::Name("Length", "The magnitude or length of the provided vector or quaternion.")
                    ScriptCanvas_Property::Output
                );
            };
        }
    }
}
