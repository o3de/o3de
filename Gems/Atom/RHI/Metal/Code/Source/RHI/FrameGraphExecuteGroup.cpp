/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/FrameGraphExecuteGroupHandler.h>

namespace AZ
{
    namespace Metal
    {
        void FrameGraphExecuteGroup::InitBase(
            Device& device,
            const RHI::GraphGroupId& groupId,
            RHI::HardwareQueueClass hardwareQueueClass)
        {
            m_device = &device;
            m_groupId = groupId;
            m_hardwareQueueClass = hardwareQueueClass;
        }

        Device& FrameGraphExecuteGroup::GetDevice() const
        {
            return *m_device;
        }

        ExecuteWorkRequest&& FrameGraphExecuteGroup::AcquireWorkRequest()
        {
            return AZStd::move(m_workRequest);
        }
    
        CommandList* FrameGraphExecuteGroup::AcquireCommandList() const
        {
            return m_device->AcquireCommandList(m_hardwareQueueClass);
        }

        RHI::HardwareQueueClass FrameGraphExecuteGroup::GetHardwareQueueClass()
        {
            return m_hardwareQueueClass;
        }
    
        const RHI::GraphGroupId& FrameGraphExecuteGroup::GetGroupId() const
        {
            return m_groupId;
        }

        void FrameGraphExecuteGroup::EncodeWaitEvents() const
        {
            AZ_Assert(m_commandBuffer->GetMtlCommandBuffer(), "Must have a valid metal command buffer");
            FenceSet compiledFences = m_device->GetCommandQueueContext().GetCompiledFences();
            for (size_t producerQueueIdx = 0; producerQueueIdx < m_workRequest.m_waitFenceValues.size(); ++producerQueueIdx)
            {
                if (const uint64_t fenceValue = m_workRequest.m_waitFenceValues[producerQueueIdx])
                {
                    const RHI::HardwareQueueClass producerQueueClass = static_cast<RHI::HardwareQueueClass>(producerQueueIdx);
                    compiledFences.GetFence(producerQueueClass).WaitOnGpu(m_commandBuffer->GetMtlCommandBuffer(), fenceValue);
                }
            }
        }
    
        void FrameGraphExecuteGroup::SetCommandBuffer(CommandQueueCommandBuffer* commandBuffer)
        {
            m_commandBuffer = commandBuffer;
        }
    
        void FrameGraphExecuteGroup::BeginInternal()
        {
            m_groupAutoreleasePool = [[NSAutoreleasePool alloc] init];
            if (m_handler)
            {
                m_handler->BeginGroup(this);
            }
            
            m_contextAutoreleasePools.resize(GetContextCount(), nil);
        }

        void FrameGraphExecuteGroup::EndInternal()
        {
            if (m_handler)
            {
                m_handler->EndGroup(this);
            }
            
            [m_groupAutoreleasePool release];
            m_groupAutoreleasePool = nil;

            AZ_Assert(!AZStd::any_of(m_contextAutoreleasePools.begin(), m_contextAutoreleasePools.end(), [](const NSAutoreleasePool* pool) { return pool != nil; }),
                      "Not all context auto release pools have been released");
            m_contextAutoreleasePools.clear();
        }
    
        void FrameGraphExecuteGroup::BeginContextInternal(
            [[maybe_unused]] RHI::FrameGraphExecuteContext& context,
            uint32_t contextIndex)
        {
            m_contextAutoreleasePools[contextIndex] = [[NSAutoreleasePool alloc] init];
        }
    
        void FrameGraphExecuteGroup::EndContextInternal(
            [[maybe_unused]] RHI::FrameGraphExecuteContext& context,
            uint32_t contextIndex)
        {
            [m_contextAutoreleasePools[contextIndex] release];
            m_contextAutoreleasePools[contextIndex] = nil;
        }
    
        void FrameGraphExecuteGroup::SetHandler(FrameGraphExecuteGroupHandler* handler)
        {
            m_handler = handler;
        }
    }
}
