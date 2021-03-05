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

#include <Include/ScriptCanvas/Libraries/Logic/Indexer.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Indexer
                : public Node
            {
                ScriptCanvas_Node(Indexer,
                    ScriptCanvas_Node::Uuid("{61E2CDC6-5CFA-47A2-8936-6A4332511E28}")
                    ScriptCanvas_Node::Description("An execution flow gate that activates each input flow in sequential order")
                    ScriptCanvas_Node::Deprecated("This node has been deprecated due to incomplete implementation and ambiguous use case")
                    ScriptCanvas_Node::Category("Logic/Deprecated")
                );

            public:

                Indexer() = default;

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
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Signaled when the node is triggered."));

            protected:
                void OnInputSignal(const SlotId& slot) override;
            };
        }
    }
}