/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TargetedSequencer.h"

#include <ScriptCanvas/Execution/ErrorBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {

            void TargetedSequencer::OnInit()
            {
                m_numOutputs = static_cast<int>(GetAllSlotsByDescriptor(SlotDescriptors::ExecutionOut()).size());
            }

            void TargetedSequencer::OnConfigured()
            {
                FixupStateNames();
            }

            void TargetedSequencer::ConfigureVisualExtensions()
            {
                {
                    VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::ExtenderSlot);

                    visualExtensions.m_name = "Add Output";
                    visualExtensions.m_tooltip = "Adds a new output to switch between.";
                    visualExtensions.m_connectionType = ConnectionType::Output;
                    visualExtensions.m_identifier = AZ::Crc32("AddOutputGroup");
                    visualExtensions.m_displayGroup = GetDisplayGroup();

                    RegisterExtension(visualExtensions);
                }
            }

            bool TargetedSequencer::CanDeleteSlot(const SlotId& slotId) const
            {
                Slot* slot = GetSlot(slotId);

                // Only remove execution out slots when we have more then 1 output slot.
                if (slot && slot->IsExecution() && slot->IsOutput())
                {
                    return m_numOutputs > 1;
                }

                return false;
            }

            SlotId TargetedSequencer::HandleExtension([[maybe_unused]] AZ::Crc32 extensionId)
            {
                ExecutionSlotConfiguration executionConfiguration(GenerateOutputName(m_numOutputs), ConnectionType::Output);

                executionConfiguration.m_addUniqueSlotByNameAndType = false;
                executionConfiguration.m_displayGroup = GetDisplayGroup();

                ++m_numOutputs;

                return AddSlot(executionConfiguration);
            }

            void TargetedSequencer::OnSlotRemoved([[maybe_unused]] const SlotId& slotId)
            {
                FixupStateNames();
            }

            AZStd::string TargetedSequencer::GenerateOutputName(int counter)
            {
                AZStd::string slotName = AZStd::string::format("Out %i", counter);
                return AZStd::move(slotName);
            }

            void TargetedSequencer::FixupStateNames()
            {
                auto outputSlots = GetAllSlotsByDescriptor(SlotDescriptors::ExecutionOut());
                m_numOutputs = static_cast<int>(outputSlots.size());

                for (int i = 0; i < outputSlots.size(); ++i)
                {
                    Slot* slot = GetSlot(outputSlots[i]->GetId());

                    if (slot)
                    {
                        slot->Rename(GenerateOutputName(i));
                    }
                }
            }
        }
    }
}
