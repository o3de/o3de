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

#include <ScriptCanvas/Core/Node.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <Include/ScriptCanvas/Libraries/Logic/Multiplexer.generated.h>


namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Multiplexer
                : public Node
            {
                ScriptCanvas_Node(Multiplexer,
                    ScriptCanvas_Node::Uuid("{2FE432CB-9969-4273-BE04-DEF949C288F0}")
                    ScriptCanvas_Node::Description("Selects one of the [In#] inputs using [Index] and sends it to [Out]")
                    ScriptCanvas_Node::Deprecated("This node has been deprecated.")
                );

            public:

                Multiplexer();

            protected:

                //The Input slots must be the first slots declared in this node, so the input slot index is the same as the slot index.
                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In0", "Input 0"));
                ScriptCanvas_In(ScriptCanvas_In::Name("In1", "Input 1"));
                ScriptCanvas_In(ScriptCanvas_In::Name("In2", "Input 2"));
                ScriptCanvas_In(ScriptCanvas_In::Name("In3", "Input 3"));
                ScriptCanvas_In(ScriptCanvas_In::Name("In4", "Input 4"));
                ScriptCanvas_In(ScriptCanvas_In::Name("In5", "Input 5"));
                ScriptCanvas_In(ScriptCanvas_In::Name("In6", "Input 6"));
                ScriptCanvas_In(ScriptCanvas_In::Name("In7", "Input 7"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Signaled when the node receives a signal from the selected index"));

                // Data
                ScriptCanvas_Property(int, 
                    ScriptCanvas_Property::Name("Index", "Select which [In#] to send to [Out].  0 <= [Index] <= 7.")
                    ScriptCanvas_Property::Input);

            protected:

                void OnInputSignal(const SlotId& slot) override;

            };
        }
    }
}