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
#include "Atom_RHI_Metal_precompiled.h"
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroupBase.h>

namespace AZ
{
    namespace Metal
    {
        void FrameGraphExecuteGroupBase::SetDevice(Device& device)
        {
            m_device = &device;
        }
        
        ExecuteWorkRequest&& FrameGraphExecuteGroupBase::AcquireWorkRequest()
        {
            return AZStd::move(m_workRequest);
        }
        
        CommandList* FrameGraphExecuteGroupBase::AcquireCommandList() const
        {
            return m_device->AcquireCommandList(m_hardwareQueueClass);
        }

        RHI::HardwareQueueClass FrameGraphExecuteGroupBase::GetHardwareQueueClass()
        {
            return m_hardwareQueueClass;
        }

        void FrameGraphExecuteGroupBase::FlushAutoreleasePool()
        {
            if (m_autoreleasePool)
            {
                [m_autoreleasePool release];
                m_autoreleasePool = nullptr;
            }
        }
        
        void FrameGraphExecuteGroupBase::CreateAutoreleasePool()
        {
            if (!m_autoreleasePool)
            {
                m_autoreleasePool = [[NSAutoreleasePool alloc] init];
            }
        }

        void FrameGraphExecuteGroupBase::EncodeWaitEvents()
        {
            AZ_Assert(m_commandBuffer.GetMtlCommandBuffer(), "Must have a valid metal command buffer");
            FenceSet compiledFences = m_device->GetCommandQueueContext().GetCompiledFences();
            for (size_t producerQueueIdx = 0; producerQueueIdx < m_workRequest.m_waitFenceValues.size(); ++producerQueueIdx)
            {
                if (const uint64_t fenceValue = m_workRequest.m_waitFenceValues[producerQueueIdx])
                {
                    const RHI::HardwareQueueClass producerQueueClass = static_cast<RHI::HardwareQueueClass>(producerQueueIdx);
                    compiledFences.GetFence(producerQueueClass).WaitOnGpu(m_commandBuffer.GetMtlCommandBuffer(), fenceValue);
                }
            }
        }
    }
}
