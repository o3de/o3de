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

#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorEmpty.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorEmpty : public Node
            {
            public:

                ScriptCanvas_Node(OperatorEmpty,
                    ScriptCanvas_Node::Name("Is Empty")
                    ScriptCanvas_Node::Uuid("{670FCD62-BAA8-4812-9CF2-C3F2EC5F54F5}")
                    ScriptCanvas_Node::Description("Returns whether the container is empty")
                    ScriptCanvas_Node::Version(1, OperatorEmptyVersionConverter)
                    ScriptCanvas_Node::Category("Containers")
                );

                static bool OperatorEmptyVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

                OperatorEmpty() = default;

                // Node
                void OnInit() override;
                ////

                void OnInputSignal(const SlotId& slotId) override;

            protected:

                ScriptCanvas_In(ScriptCanvas_In::Name("In", ""));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("True", "The container is empty"));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("False", "The container is not empty"));

                ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Container
                    , ScriptCanvas::ConnectionType::Input
                    , ScriptCanvas_DynamicDataSlot::Name("Source", "The container to check if its empty.")
                    , ScriptCanvas_DynamicDataSlot::SupportsMethodContractTag("Empty")
                );

                ScriptCanvas_Property(bool,
                    ScriptCanvas_Property::Name("Is Empty", "True if the container is empty, false if it's not.")
                    ScriptCanvas_Property::Output
                );
            };
        }
    }
}