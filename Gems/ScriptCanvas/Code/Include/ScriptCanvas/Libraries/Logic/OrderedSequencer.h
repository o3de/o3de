/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

                bool CanDeleteSlot(const SlotId& slotId) const override;

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override;

                SlotId HandleExtension(AZ::Crc32 extensionId) override;

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
