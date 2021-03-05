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

#include <Include/ScriptCanvas/Libraries/Logic/IsNull.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class IsNull
                : public Node
            {
            public:
                ScriptCanvas_Node(IsNull,
                    ScriptCanvas_Node::Name("Is Null", "Evaluates reference types for null.")
                    ScriptCanvas_Node::Uuid("{760CE936-7059-42A3-A177-530A662E4ECF}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/IsNull.png")
                    ScriptCanvas_Node::Version(0)
                );

                IsNull();

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("True", "Signaled if the reference provided is null."));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("False", "Signaled if the reference provided is not null."));

            protected:
                void OnInit() override;

                void OnInputSignal(const SlotId&) override;

            };
        }
    }
}