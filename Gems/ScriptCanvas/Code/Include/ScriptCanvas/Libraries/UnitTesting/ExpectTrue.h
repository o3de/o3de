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
#include <Include/ScriptCanvas/Libraries/UnitTesting/ExpectTrue.generated.h>
#include <Include/ScriptCanvas/Libraries/UnitTesting/UnitTesting.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            class ExpectTrue
                : public Node
            {
            public:
                ScriptCanvas_Node(ExpectTrue,
                    ScriptCanvas_Node::Name("Expect True", "Expects a value to be true")
                    ScriptCanvas_Node::Uuid("{88F9BE2D-F591-45AD-9682-FBB67C39C504}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/ExpectTrue.png")
                    ScriptCanvas_Node::Version(1, ScriptCanvas::UnitTesting::ExpectBooleanVersioner)
                );

                void OnInit() override;
                void OnInputSignal(const SlotId& slotId) override;

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

            private:
                ScriptCanvas_Property(bool,
                    ScriptCanvas_Property::Name("Candidate", "a value that must be true")
                    ScriptCanvas_Property::Input);
                
                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Report", "additional notes for the test report")
                    ScriptCanvas_Property::Input);
            }; // class ExpectTrue
        } // namespace UnitTesting
    } // namespace Nodes
} // namespace ScriptCanvas