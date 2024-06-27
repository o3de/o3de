/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom_RHI_Vulkan_Platform.h>
#include <RHI/IndirectBufferSignature.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<IndirectBufferSignature> IndirectBufferSignature::Create()
        {
            return aznew IndirectBufferSignature();
        }

        RHI::ResultCode IndirectBufferSignature::InitInternal([[maybe_unused]] RHI::Device& deviceBase, const RHI::DeviceIndirectBufferSignatureDescriptor& descriptor)
        {
            // Vulkan doesn't have an object to represent an indirect buffer signature.
            // We just calculate the offsets of the commands and the stride of the whole sequence.
            auto commands = descriptor.m_layout.GetCommands();

            m_stride = 0;
            m_offsets.resize(commands.size());
            for (uint32_t i = 0; i < commands.size(); ++i)
            {
                m_offsets[i] = m_stride;
                const RHI::IndirectCommandDescriptor& command = commands[i];
                switch (command.m_type)
                {
                case RHI::IndirectCommandType::Draw:
                    m_stride += sizeof(VkDrawIndirectCommand);
                    break;
                case RHI::IndirectCommandType::DrawIndexed:
                    m_stride += sizeof(VkDrawIndexedIndirectCommand);
                    break;
                case RHI::IndirectCommandType::Dispatch:
                    m_stride += sizeof(VkDispatchIndirectCommand);
                    break;
                case RHI::IndirectCommandType::DispatchRays:
                    m_stride += sizeof(VkTraceRaysIndirectCommandKHR);
                    break;
                default:
                    // Unsupported command types.
                    AZ_Assert(false, "Unsupported indirect command type (%d)", command.m_type);
                    return RHI::ResultCode::InvalidArgument;
                }
            }

            return RHI::ResultCode::Success;
        }

        uint32_t IndirectBufferSignature::GetByteStrideInternal() const
        {
            return m_stride;
        }

        void IndirectBufferSignature::ShutdownInternal()
        {
            m_stride = 0;
        }

        uint32_t IndirectBufferSignature::GetOffsetInternal(RHI::IndirectCommandIndex index) const
        {
            return m_offsets[index.GetIndex()];
        }
    }
}
