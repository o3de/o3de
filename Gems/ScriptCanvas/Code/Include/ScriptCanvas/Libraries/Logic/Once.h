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

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Libraries/Logic/Boolean.h>

#include <Include/ScriptCanvas/Libraries/Logic/Once.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Once
                : public Node
            {
                ScriptCanvas_Node(Once,
                    ScriptCanvas_Node::Uuid("{0E37D3CA-2862-4667-BFDB-7393DD48241B}")
                    ScriptCanvas_Node::Description("An execution flow gate that will only activate once. The gate will reopen if it receives a Reset pulse.")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Print.png")
                    ScriptCanvas_Node::Version(0)
                );

            public:

                Once();

            private:

                bool m_resetStatus;

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));
                ScriptCanvas_In(ScriptCanvas_In::Name("Reset", "Reset signal"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Output signal"));

                void OnInputSignal(const SlotId& slotId) override;
            };
        }
    }
}
