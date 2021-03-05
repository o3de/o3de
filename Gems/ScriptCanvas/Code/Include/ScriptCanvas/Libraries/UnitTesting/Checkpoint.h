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
#include <Include/ScriptCanvas/Libraries/UnitTesting/Checkpoint.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            class Checkpoint
                : public Node
            {
            public:
                ScriptCanvas_Node(Checkpoint,
                    ScriptCanvas_Node::Name("Checkpoint", "Add a progress checkpoint for test debugging")
                    ScriptCanvas_Node::Uuid("{E65449D2-45A9-402B-ADF7-4E4F27A99245}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Checkpoint.png")
                    ScriptCanvas_Node::Version(0)
                );

                void OnInputSignal(const SlotId& slotId) override;

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

            private:
                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Report", "additional notes for the test report")
                    ScriptCanvas_Property::Input);
            }; // class Checkpoint
        } // namespace UnitTesting
    } // namespace Nodes
} // namespace ScriptCanvas
