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
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <Include/ScriptCanvas/Libraries/Core/Start.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class Start 
                : public Node
            {
            public:

            public:
                ScriptCanvas_Node(Start,
                    ScriptCanvas_Node::Name("On Graph Start", "Starts executing the graph when the entity that owns the graph is fully activated.")
                    ScriptCanvas_Node::Uuid("{F200B22A-5903-483A-BF63-5241BC03632B}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Start.png")
                    ScriptCanvas_Node::Version(2)
                    ScriptCanvas_Node::Category("Timing")
                    ScriptCanvas_Node::GraphEntryPoint(true)
                );

                void OnInputSignal(const SlotId&) override;
                
                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Signaled when the entity that owns this graph is fully activated."));

            };
        }
    }
}