/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/Specific/SelectorPass.h>


namespace AZ
{
    namespace RPI
    {
        Ptr<SelectorPass> SelectorPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<SelectorPass> pass = aznew SelectorPass(descriptor);
            return pass;
        }

        SelectorPass::SelectorPass(const PassDescriptor& descriptor)
            : Pass(descriptor)
        {
            uint32_t outputSlotCount = 0;
            for (const PassSlot& slot : m_template->m_slots)
            {
                if (slot.m_slotType == PassSlotType::Output)
                {
                    outputSlotCount++;
                }
            }
            m_connections.resize(outputSlotCount);

            // Maybe move this default mapping to pass data.
            uint32_t inputIndex = 0;
            for (auto& connection : m_connections)
            {
                connection = inputIndex++;
            }
        }

        void SelectorPass::BuildInternal()
        {
            // Update output connections based on m_connections
            // This need to be done after BuildAttachment is finished
            for (uint32_t outputSlotIndex = 0; outputSlotIndex < m_connections.size(); outputSlotIndex++)
            {
                PassAttachmentBinding& outputBinding = GetOutputBinding(outputSlotIndex);
                PassAttachmentBinding& inputBinding = GetInputBinding(m_connections[outputSlotIndex]);
                outputBinding.m_attachment = inputBinding.m_attachment;
            }
        }

        void SelectorPass::Connect(uint32_t inputSlotIndex, uint32_t outputSlotIndex)
        {
            if (outputSlotIndex >= m_connections.size())
            {
                AZ_Assert(false, "outputSlotIndex %d doesn't exist", outputSlotIndex);
                return;
            }
            if (inputSlotIndex >= GetInputCount())
            {
                AZ_Assert(false, "inputSlotIndex %d doesn't exist", inputSlotIndex);
                return;
            }

            m_connections[outputSlotIndex] = inputSlotIndex;

            // Queue to rebuild attachment connections
            QueueForBuildAndInitialization();
        }

        void SelectorPass::Connect(const AZ::Name& inputSlot, const AZ::Name& outputSlot)
        {
            // Check whether both inputSlot or outputSlot are valid
            uint32_t outputIdx, inputIdx;
            for (outputIdx = 0; outputIdx < GetOutputCount(); ++outputIdx)
            {
                const PassAttachmentBinding& binding = GetOutputBinding(outputIdx);
                if (outputSlot == binding.m_name)
                {
                    break;
                }
            }

            if (outputIdx == GetOutputCount())
            {
                AZ_Assert(false, "Can't find output slot %s", outputSlot.GetCStr());
                return;
            }

            for (inputIdx = 0; inputIdx < GetInputCount(); ++inputIdx)
            {
                const PassAttachmentBinding& binding = GetInputBinding(inputIdx);
                if (inputSlot == binding.m_name)
                {
                    break;
                }
            }

            if (inputIdx == GetInputCount())
            {
                AZ_Assert(false, "Can't find input slot %s", inputSlot.GetCStr());
                return;
            }

            // Add connection to the map
            m_connections[outputIdx] = inputIdx;

            // Queue to rebuild attachment connections
            QueueForBuildAndInitialization();
        }

    } // namespace RPI
} // namespace AZ
