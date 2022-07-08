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

                bool CanDeleteSlot(const SlotId& slotId) const override;

                SlotId HandleExtension(AZ::Crc32 extensionId) override;

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override;

            protected:

                AZStd::string GetDisplayGroup() const { return "OutputGroup"; }
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
