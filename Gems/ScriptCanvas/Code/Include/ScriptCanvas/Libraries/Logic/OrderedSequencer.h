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

#include <Include/ScriptCanvas/Libraries/Logic/OrderedSequencer.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! Triggers the execution outputs in the specified ordered. The next line will trigger once the first line reaches a break in execution, either through latent node or a terminal endpoint.
            class OrderedSequencer
                : public Node
            {
 
            public:

                SCRIPTCANVAS_NODE(OrderedSequencer);

                OrderedSequencer();

                bool CanDeleteSlot(const SlotId& slotId) const;

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override;

                SlotId HandleExtension(AZ::Crc32 extensionId);

                void OnInit() override;

                void OnConfigured() override;

                void ConfigureVisualExtensions() override;

                

            protected:
            
                AZStd::string GetDisplayGroup() const { return "OutputGroup"; }

            protected:

                void OnSlotRemoved(const SlotId& slotId) override;

            private:

                AZStd::string GenerateOutputName(int counter) const;
                void FixupStateNames();
                
                int m_numOutputs;
                
                AZStd::vector< SlotId > m_orderedOutputSlots;
            };
        }
    }   
}
