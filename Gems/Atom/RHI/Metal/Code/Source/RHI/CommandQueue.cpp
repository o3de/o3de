/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/CommandQueue.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>
#include <RHI/SwapChain.h>

#include <AzCore/Debug/Timer.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<CommandQueue> CommandQueue::Create()
        {
            return aznew CommandQueue();
        }

        id<MTLCommandQueue> CommandQueue::GetPlatformQueue() const
        {
            return m_hwQueue;
        }

        CommandQueueCommandBuffer& CommandQueue::GetCommandBuffer()
        {
            return m_commandBuffer;
        }

        RHI::ResultCode CommandQueue::InitInternal(RHI::Device& deviceBase, const RHI::CommandQueueDescriptor& descriptor)
        {
            RHI::DeviceObject::Init(deviceBase);
            
            auto& device = static_cast<Device&>(deviceBase);
            m_hwQueue = [device.GetMtlDevice() newCommandQueue];
            AZ_Assert(m_hwQueue, "Could not create the metal queue");
            
            switch (descriptor.m_hardwareQueueClass)
            {
            case RHI::HardwareQueueClass::Copy:
                m_hwQueue.label = @"Copy Queue";
                break;

            case RHI::HardwareQueueClass::Compute:
                m_hwQueue.label = @"Compute Queue";
                break;

            case RHI::HardwareQueueClass::Graphics:
                m_hwQueue.label = @"Graphics Queue";
                break;
            }
            
            m_commandBuffer.Init(m_hwQueue);
            return RHI::ResultCode::Success;
        }

        void CommandQueue::ShutdownInternal()
        {
            m_hwQueue = nil;            
        }

        void CommandQueue::QueueGpuSignal(Fence& fence)
        {
            QueueGpuSignal(fence, fence.GetPendingValue());
        }

        void CommandQueue::QueueGpuSignal(Fence& fence, uint64_t signalValue)
        {
            QueueCommand([this, &fence, signalValue](void* queue)
            {
                //Autoreleasepool is to ensure that the driver is not leaking memory related to the command buffer and encoder
                @autoreleasepool
                {
                    CommandQueue* commandQueue = static_cast<CommandQueue*>(queue);
                    fence.SignalFromGpu(commandQueue->GetCommandBuffer().AcquireMTLCommandBuffer(), signalValue);
                    m_commandBuffer.CommitMetalCommandBuffer();
                }
            });
        }

        void CommandQueue::ExecuteWork(const RHI::ExecuteWorkRequest& rhiRequest)
        {
            auto& device = static_cast<Device&>(GetDevice());
            const ExecuteWorkRequest& request = static_cast<const ExecuteWorkRequest&>(rhiRequest);
            
            bool haveEncodedList = false;
            for (uint32_t commandListIdx = 0; commandListIdx < static_cast<uint32_t>(request.m_commandLists.size()); ++commandListIdx)
            {
                CommandList& commandList = *request.m_commandLists[commandListIdx];
                if(commandList.IsEncoded())
                {
                    haveEncodedList = true;
                    break;
                }
            }
    
            if(haveEncodedList)
            {
                //Since we call executework on all the groups in the correct order we safely call enqueue here to ensure
                //that we reserve a place for the command buffer in the correct order on the associated hw command queue.
                id <MTLCommandBuffer> workRequestCommandBuffer = request.m_commandBuffer->GetMtlCommandBuffer();
                [workRequestCommandBuffer enqueue];
                
                CommandQueueContext& context = device.GetCommandQueueContext();
                const FenceSet& compiledFences = context.GetCompiledFences();
                
                QueueCommand([=](void* unused)
                {
                     //Autoreleasepool is to ensure that the driver is not leaking memory related to the command buffer and encoder
                     @autoreleasepool
                     {
                         AZ_PROFILE_SCOPE(RHI, "ExecuteWork");
                         AZ::Debug::ScopedTimer executionTimer(m_lastExecuteDuration);
                         
                         if (request.m_signalFenceValue > 0)
                         {
                             compiledFences.GetFence(GetDescriptor().m_hardwareQueueClass).SignalFromGpu(workRequestCommandBuffer, request.m_signalFenceValue);
                         }

                         for (Fence* fence : request.m_scopeFencesToSignal)
                         {
                             fence->SignalFromGpu(workRequestCommandBuffer);
                         }
                         
                         {
                             AZ::Debug::ScopedTimer presentTimer(m_lastPresentDuration);
                         
                             for (RHI::SwapChain* swapChain : request.m_swapChainsToPresent)
                             {
                                 
                                 static_cast<SwapChain*>(swapChain)->SetCommandBuffer(workRequestCommandBuffer);
                                 swapChain->Present();
                             }
                         }
                         
                         //Commit the command buffer to the command queue
                         request.m_commandBuffer->CommitMetalCommandBuffer();                         
                     }
                 });
            }                        
        }

        void CommandQueue::WaitForIdle()
        {
            auto& device = static_cast<Device&>(GetDevice());
            Fence fence;
            fence.Init(&device, RHI::FenceState::Reset);
            QueueGpuSignal(fence);
            FlushCommands();
            fence.WaitOnCpu();
        }

        void* CommandQueue::GetNativeQueue()
        {
            return this;
        }
        
        void CommandQueue::ClearTimers()
        {
            m_lastExecuteDuration = 0;
        }
    
        AZStd::sys_time_t CommandQueue::GetLastExecuteDuration() const
        {
            return m_lastExecuteDuration;
        }

        AZStd::sys_time_t CommandQueue::GetLastPresentDuration() const
        {
            return m_lastPresentDuration;
        }
    }
}
