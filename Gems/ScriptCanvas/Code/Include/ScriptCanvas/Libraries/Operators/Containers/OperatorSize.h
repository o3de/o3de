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

#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorSize.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorSize : public Node
            {
            public:
                ScriptCanvas_Node(OperatorSize,
                    ScriptCanvas_Node::Name("Get Size")
                    ScriptCanvas_Node::Uuid("{981EA18E-F421-4B39-9FF7-27322F3E8B14}")
                    ScriptCanvas_Node::Description("Get the number of elements in the specified container")
                    ScriptCanvas_Node::Version(1, OperatorSizeVersionConverter)
                    ScriptCanvas_Node::Category("Containers")
                );

                static bool OperatorSizeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

                OperatorSize() = default;

                // Node
                void OnInit() override;
                ////

                void OnInputSignal(const SlotId& slotId) override;

            protected:

                ScriptCanvas_In(ScriptCanvas_In::Name("In", ""));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

                ScriptCanvas_DynamicDataSlot(  ScriptCanvas::DynamicDataType::Container
                                             , ScriptCanvas::ConnectionType::Input
                                             , ScriptCanvas_DynamicDataSlot::Name("Source", "The container to get the size of.")
                                             , ScriptCanvas_DynamicDataSlot::SupportsMethodContractTag("Size")
                                        );

                ScriptCanvas_Property(int,
                                      ScriptCanvas_Property::Name("Size", "The size of the specified container")
                                      ScriptCanvas_Property::Output
                                    );
            }; 
        }
    }
}