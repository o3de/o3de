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

#include <Include/ScriptCanvas/Libraries/Logic/TargetedSequencer.generated.h>


namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class TargetedSequencer
                : public Node
            {
                ScriptCanvas_Node(TargetedSequencer,
                    ScriptCanvas_Node::Name("Switch")
                    ScriptCanvas_Node::Uuid("{E1B5F3F8-AFEE-42C9-A22C-CB93F8281CC4}")
                    ScriptCanvas_Node::Description("Triggers the specified output when triggered.")
                );

            public:

                TargetedSequencer();

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

                // Data
                ScriptCanvas_Property(int, 
                                    ScriptCanvas_Property::Name("Index", "Select which [Out#] to trigger.")
                                    ScriptCanvas_Property::Input
                                    ScriptCanvas_Property::DisplayGroup("OutputGroup")
                                    );

            protected:

                void OnInputSignal(const SlotId& slot) override;                
                void OnSlotRemoved(const SlotId& slotId) override;

            private:

                AZStd::string GenerateOutputName(int counter);
                void FixupStateNames();
                
                int m_numOutputs;
            };
        }
    }
}