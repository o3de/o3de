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
#include <ScriptCanvas/Core/GraphBus.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <Include/ScriptCanvas/Libraries/Logic/OrderedSequencer.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class OrderedSequencer
                : public Node                
            {
                ScriptCanvas_Node(OrderedSequencer,
                    ScriptCanvas_Node::Name("Ordered Sequencer")
                    ScriptCanvas_Node::Uuid("{BAFDA139-49A8-453B-A556-D4F4BA213B5C}"),
                    ScriptCanvas_Node::Description("Triggers the execution outputs in the specified ordered. The next line will trigger once the first line reaches a break in execution(either through latent node, or a terminal endpoint)")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Logic")
                );
                
            public:

                OrderedSequencer();

                void OnInit() override;
                void OnConfigured() override;
                void ConfigureVisualExtensions() override;
                
                bool CanDeleteSlot(const SlotId& slotId) const;

                SlotId HandleExtension(AZ::Crc32 extensionId);        

            protected:
            
                AZStd::string GetDisplayGroup() const { return "OutputGroup"; }

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", ""));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out 0", "Output 0")
                                 ScriptCanvas_Out::DisplayGroup("OutputGroup")
                                );

            protected:

                void OnInputSignal(const SlotId& slot) override;                
                void OnSlotRemoved(const SlotId& slotId) override;

            private:

                AZStd::string GenerateOutputName(int counter);
                void FixupStateNames();
                
                int m_numOutputs;
                
                AZStd::vector< SlotId > m_orderedOutputSlots;
            };
        }
    }   
}