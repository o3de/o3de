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

#include <Include/ScriptCanvas/Libraries/Logic/Cycle.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! Performs a cyclic execution signaling
            class Cycle
                : public Node
            {

            public:

                SCRIPTCANVAS_NODE(Cycle);

                Cycle();

                void OnInit() override;
                void OnActivate() override;

                void OnConfigured() override;
                void ConfigureVisualExtensions() override;

                bool CanDeleteSlot(const SlotId& slotId) const;

                SlotId HandleExtension(AZ::Crc32 extensionId);

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                SlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override;

                bool IsSupportedByNewBackend() const override { return true; }

            protected:

                AZStd::string GetDisplayGroup() const { return "OutputGroup"; }

                void OnInputSignal(const SlotId& slot) override;
                void OnSlotRemoved(const SlotId& slotId) override;

            private:

                AZStd::string GenerateOutputName(int counter);
                void FixupStateNames();

                int m_numOutputs;
                
                size_t m_executionSlot;

                AZStd::vector< SlotId > m_orderedOutputSlots;
            };
        }
    }
}
