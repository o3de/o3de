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

#include <Include/ScriptCanvas/Libraries/Logic/Sequencer.generated.h>


namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Sequencer
                : public Node
            {
                ScriptCanvas_Node(Sequencer,
                    ScriptCanvas_Node::Uuid("{CB98B828-BF86-4623-BF73-396A68FA386A}")
                    ScriptCanvas_Node::Description("Trigger one of the outputs in sequential order for each input activation.")
                    ScriptCanvas_Node::Deprecated("This node has been deprecated since it combined two node functions in an odd way. It has been replaced by the Ordered Sequencer and the Switch nodes.")
                );

            public:

                Sequencer();

                enum Order
                {
                    Forward = 0,
                    Backward
                };

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));
                ScriptCanvas_In(ScriptCanvas_In::Name("Next", "When next is activated, it enables the next output"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out0", "Output 0"));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out1", "Output 1"));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out2", "Output 2"));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out3", "Output 3"));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out4", "Output 4"));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out5", "Output 5"));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out6", "Output 6"));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out7", "Output 7"));

                // Data
                ScriptCanvas_Property(int, 
                    ScriptCanvas_Property::Name("Index", "Select which [Out#] to active.  0 <= [Index] <= 7.")
                    ScriptCanvas_Property::Input);

                ScriptCanvas_Property(int, 
                    ScriptCanvas_Property::Name("Order", "0 represents 'forward' and 1 represents 'backward', default is 0")
                    ScriptCanvas_Property::Input);

                // temps
                int m_order;
                int m_selectedIndex;

            protected:

                void OnInputSignal(const SlotId& slot) override;

            private:
                int m_currentIndex;
                bool m_outputIsValid;

                SlotId GetCurrentSlotId() const;
            };
        }
    }
}