/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/SwapChain.h>

#include <AzCore/Debug/Timer.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<CommandQueue> CommandQueue::Create()
        {
            return aznew CommandQueue();
        }

        RHI::ResultCode CommandQueue::InitInternal(RHI::Device& deviceBase, const RHI::CommandQueueDescriptor& descriptor)
        {
            DeviceObject::Init(deviceBase);

            const CommandQueueDescriptor& commandQueueDesc = static_cast<const CommandQueueDescriptor&>(descriptor);

            m_queueDescriptor.m_familyIndex = static_cast<uint32_t>(commandQueueDesc.m_hardwareQueueClass);
            m_queueDescriptor.m_queueIndex = commandQueueDesc.m_queueIndex;
            m_queueDescriptor.m_commandQueue = this;

            m_queue = aznew Queue();
            const RHI::ResultCode result = m_queue->Init(deviceBase, m_queueDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            m_supportedStagesMask = CalculateSupportedPipelineStages();
            return result;
        }

        void CommandQueue::ExecuteWork(const RHI::ExecuteWorkRequest& rhiRequest)
        {
            const ExecuteWorkRequest& request = static_cast<const ExecuteWorkRequest&>(rhiRequest);
            QueueCommand([=](void* queue) 
            {
                AZ_PROFILE_SCOPE(RHI, "ExecuteWork");
                AZ::Debug::ScopedTimer executionTimer(m_lastExecuteDuration);

                Queue* vulkanQueue = static_cast<Queue*>(queue);

                if (!request.m_debugLabel.empty())
                {
                    vulkanQueue->BeginDebugLabel(request.m_debugLabel.c_str());
                }

                AZStd::vector<VkSemaphore> semaphoresToSignal(request.m_semaphoresToSignal.size());
                for (size_t index = 0; index < request.m_semaphoresToSignal.size(); ++index)
                {
                    semaphoresToSignal[index] = request.m_semaphoresToSignal[index]->GetNativeSemaphore();
                }

                Fence* fenceToSignal = nullptr;
                if (!request.m_fencesToSignal.empty())
                {
                    // Only the first fence is added to the submit command buffer call
                    // since Vulkan can only signal one fence per submit.
                    RHI::Ptr<Fence> fence = request.m_fencesToSignal.front();
                    fenceToSignal = fence.get();
                }

                // Submit commands to queue for the current frame.
                vulkanQueue->SubmitCommandBuffers({request.m_commandList}, request.m_semaphoresToWait, request.m_semaphoresToSignal, fenceToSignal);
                // Need to signal all the other fences (other than the first one)
                for (size_t i = 1; i < request.m_fencesToSignal.size(); ++ i)
                {
                   const auto& fence = request.m_fencesToSignal[i];
                   Signal(*fence);
                }

                {
                    AZ::Debug::ScopedTimer presentTimer(m_lastPresentDuration);

                    // present the image of the current frame.
                    for (RHI::SwapChain* swapChain : request.m_swapChainsToPresent)
                    {
                        swapChain->Present();
                    }
                }

                if (!request.m_debugLabel.empty())
                {
                    vulkanQueue->EndDebugLabel();
                }
            });
        }
        
        void CommandQueue::Signal(Fence& fence)
        {
            // The queue doesn't have an explicit way to signal a fence, so
            // we submit an empty work batch with only a fence to signal.
            QueueCommand([&fence](void* queue)
            {
                Queue* vulkanQueue = static_cast<Queue*>(queue);
                vulkanQueue->SubmitCommandBuffers(
                    AZStd::vector<RHI::Ptr<CommandList>>(),
                    AZStd::vector<Semaphore::WaitSemaphore>(), 
                    AZStd::vector<RHI::Ptr<Semaphore>>(), 
                    &fence);
            });
        }
        
        void CommandQueue::WaitForIdle()
        {
            m_queue->WaitForIdle();
        }

        void CommandQueue::ShutdownInternal()
        {
            if (m_queue)
            {
                m_queue = nullptr;
            }
        }
        
        const Queue::Descriptor& CommandQueue::GetQueueDescriptor() const
        {
            return m_queueDescriptor;
        }
        
        VkPipelineStageFlags CommandQueue::GetSupportedPipelineStages() const
        {
            return m_supportedStagesMask;
        }

        QueueId CommandQueue::GetId() const
        {
            return m_queue->GetId();
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
        
        VkPipelineStageFlags CommandQueue::CalculateSupportedPipelineStages() const
        {
            auto& device = static_cast<Device&>(GetDevice());
            // These stages don't need any special queue to be supported.
            VkPipelineStageFlags flags = 
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
                VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            const VkQueueFamilyProperties& properties = device.GetQueueFamilyProperties()[m_queueDescriptor.m_familyIndex];
            auto queueFlags = properties.queueFlags;
            if (RHI::CheckBitsAny(queueFlags, static_cast<VkQueueFlags>(VK_QUEUE_GRAPHICS_BIT)))
            {
                flags |= Vulkan::GetSupportedPipelineStages(RHI::PipelineStateType::Draw);
                // Graphic queues support transfer operations (but they are not obligated to have the transfer bit)
                queueFlags |= VK_QUEUE_TRANSFER_BIT;
            }

            if (RHI::CheckBitsAny(queueFlags, static_cast<VkQueueFlags>(VK_QUEUE_COMPUTE_BIT)))
            {
                flags |= Vulkan::GetSupportedPipelineStages(RHI::PipelineStateType::Dispatch);
                // Compute queues support transfer operations (but they are not obligated to have the transfer bit)
                queueFlags |= VK_QUEUE_TRANSFER_BIT;
            }

            if (RHI::CheckBitsAny(queueFlags, static_cast<VkQueueFlags>(VK_QUEUE_TRANSFER_BIT)))
            {
                flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
            }

            return RHI::FilterBits(flags, device.GetSupportedPipelineStageFlags());
        }

        void* CommandQueue::GetNativeQueue()
        {
            return m_queue.get();
        }
    }
}
