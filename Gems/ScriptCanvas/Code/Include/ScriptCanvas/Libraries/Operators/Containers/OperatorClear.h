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

#include <ScriptCanvas/Libraries/Operators/Operator.h>

#include <ScriptCanvas/Core/Node.h>
#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorClear.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorClear : public Node
            {
            public:
                ScriptCanvas_Node(OperatorClear,
                    ScriptCanvas_Node::Name("Clear All Elements")
                    ScriptCanvas_Node::Uuid("{26DDC284-BD01-471E-BA8F-36C3A1ADA169}")
                    ScriptCanvas_Node::Description("Eliminates all the elements in the container")
                    ScriptCanvas_Node::Version(1, OperatorClearVersionConverter)
                    ScriptCanvas_Node::Category("Containers")
                );

                enum Version
                {
                    InitialVersion = 0,
                    RemoveOperatorBase,

                    Current
                };

                static bool OperatorClearVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                OperatorClear() = default;

                void OnInit() override;

                ScriptCanvas_In(ScriptCanvas_In::Name("In", ""));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));
                
                // Because the slots are grouped. Only one needs to be configured with the data restrictions and the others will all get them.
                ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Container,
                                             ScriptCanvas::ConnectionType::Input,
                                             ScriptCanvas_DynamicDataSlot::Name("Source", "The container to be cleared from the node.")
                                             ScriptCanvas_DynamicDataSlot::DynamicGroup("ContainerGroup")
                                             ScriptCanvas::SupportsMethodContract("Clear")
                                            );

                ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Container,
                                             ScriptCanvas::ConnectionType::Output,
                                             ScriptCanvas_DynamicDataSlot::Name("Container", "The container, now cleared of elements.")
                                             ScriptCanvas_DynamicDataSlot::DynamicGroup("ContainerGroup")
                                             ScriptCanvas::SupportsMethodContract("Clear")
                                            );

            protected:

                void OnInputSignal(const SlotId& slotId) override;
            };

        }
    }
}
