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
#include <Include/ScriptCanvas/Libraries/UnitTesting/ExpectEqual.generated.h>
#include <Include/ScriptCanvas/Libraries/UnitTesting/UnitTesting.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            class ExpectEqual
                : public Node
            {
            public:
                ScriptCanvas_Node(ExpectEqual,
                    ScriptCanvas_Node::Name("Expect Equal", "Expects lhs equal to rhs")
                    ScriptCanvas_Node::Uuid("{856DB72A-48CB-4142-A032-1253D3AB8BEC}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/ExpectEqual.png")
                    ScriptCanvas_Node::Version(1, ScriptCanvas::UnitTesting::ExpectComparisonVersioner)
                );

                void OnInit() override;
                void OnInputSignal(const SlotId& slotId) override;

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

            private:
                ScriptCanvas_DynamicDataSlot(DynamicDataType::Any,
                                             ConnectionType::Input,
                                             ScriptCanvas_DynamicDataSlot::Name("Candidate", "left of ==")
                                             ScriptCanvas_DynamicDataSlot::DynamicGroup("DynamicGroup")
                                            );

                ScriptCanvas_DynamicDataSlot(DynamicDataType::Any,
                                             ConnectionType::Input,
                                             ScriptCanvas_DynamicDataSlot::Name("Reference", "right of ==")
                                             ScriptCanvas_DynamicDataSlot::DynamicGroup("DynamicGroup")
                                         );

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Report", "additional notes for the test report")
                    ScriptCanvas_Property::Input);
            }; // class ExpectEqual
        } // namespace UnitTesting
    } // namespace Nodes
} // namespace ScriptCanvas