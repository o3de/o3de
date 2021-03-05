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

#include <Include/ScriptCanvas/Libraries/Logic/Gate.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Gate
                : public Node
            {
            public:

                  ScriptCanvas_Node(Gate,
                      ScriptCanvas_Node::Name("If", "An execution flow gate that continues True if the Condition is True, otherwise continues False")
                      ScriptCanvas_Node::Uuid("{F19CC10A-02FD-4E75-ADAA-9CFBD8A4E2F8}")
                      ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Print.png")
                      ScriptCanvas_Node::Version(0)
                  );
 
                 Gate();

                 // Inputs
                  ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));
  
                  // Outputs
                  ScriptCanvas_Out(ScriptCanvas_Out::Name("True", "Signaled if the condition provided evaluates to true."));
                  ScriptCanvas_Out(ScriptCanvas_Out::Name("False", "Signaled if the condition provided evaluates to false."));

             private:
  
                  // Data
                 ScriptCanvas_Property(bool,
                     ScriptCanvas_Property::Name("Condition", "If true the node will signal the Output and proceed execution")
                     ScriptCanvas_Property::Transient
                     ScriptCanvas_Property::Input);
                  
                 bool m_condition;
 
             protected:

                 void OnInputSignal(const SlotId&) override;
 
 
            };
        }
    }
}