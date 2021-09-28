/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>

#include <Include/ScriptCanvas/Libraries/Logic/TargetedSequencer.generated.h>


namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            // Will trigger the specified execution upon being triggered
            class TargetedSequencer
                : public Node
            {
                SCRIPTCANVAS_NODE(TargetedSequencer);

            public:

                TargetedSequencer();

                void OnInit() override;
                void OnConfigured() override;
                void ConfigureVisualExtensions() override;

                bool CanDeleteSlot(const SlotId& slotId) const override;

                SlotId HandleExtension(AZ::Crc32 extensionId) override;

                // Script Canvas Translation...
                bool IsSwitchStatement() const override { return true; }

                AZ::Outcome<DependencyReport, void> GetDependencies() const override
                {
                    return AZ::Success(DependencyReport{});
                }

                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot&, CombinedSlotType targetSlotType, const Slot*) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }
                //////////////////////////////////////////////////////////////////////////

            protected:

                AZStd::string GetDisplayGroup() const { return "OutputGroup"; }

                void OnSlotRemoved(const SlotId& slotId) override;

            private:

                AZStd::string GenerateOutputName(int counter);
                void FixupStateNames();

                int m_numOutputs;
            };
        }
    }
}
