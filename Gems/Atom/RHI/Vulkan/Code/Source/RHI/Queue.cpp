/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/algorithm.h>
#include <AzCore/std/iterator.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Semaphore.h>
#include <RHI/Fence.h>
#include <RHI/Queue.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode Queue::Init(RHI::Device& deviceBase, const Descriptor& descriptor)
        {
            auto& device = static_cast<Device&>(deviceBase);
            Base::Init(deviceBase);
            m_descriptor = descriptor;

            device.GetContext().GetDeviceQueue(
                device.GetNativeDevice(), m_descriptor.m_familyIndex, descriptor.m_queueIndex, &m_nativeQueue);
            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        VkQueue Queue::GetNativeQueue() const
        {
            return m_nativeQueue;
        }

        RHI::ResultCode Queue::SubmitCommandBuffers(
            const AZStd::vector<RHI::Ptr<CommandList>>& commandBuffers, 
            const AZStd::vector<Semaphore::WaitSemaphore>& waitSemaphoresInfo,
            const AZStd::vector< RHI::Ptr<Semaphore>>& semaphoresToSignal,
            Fence* fenceToSignal)
        {
            AZStd::vector<VkCommandBuffer> vkCommandBuffers;
            AZStd::vector<VkSemaphore> vkWaitSemaphoreVector; // vulkan.h has a #define called vkWaitSemaphores, so we name this differently
            AZStd::vector<VkPipelineStageFlags> vkWaitPipelineStages;
            AZStd::vector<VkSemaphore> vkSignalSemaphores;
            VkSubmitInfo submitInfo;
            uint32_t submitCount = 0;
            if (!commandBuffers.empty() ||
                !waitSemaphoresInfo.empty() ||
                !semaphoresToSignal.empty())
            {
                vkCommandBuffers.reserve(commandBuffers.size());
                AZStd::transform(commandBuffers.begin(), commandBuffers.end(), AZStd::back_inserter(vkCommandBuffers), [&](const auto& item)
                { 
                    return item->GetNativeCommandBuffer(); 
                });
                vkSignalSemaphores.reserve(semaphoresToSignal.size());
                AZStd::transform(semaphoresToSignal.begin(), semaphoresToSignal.end(), AZStd::back_inserter(vkSignalSemaphores), [&](const auto& item)
                { 
                    return item->GetNativeSemaphore(); 
                });
                vkWaitPipelineStages.reserve(waitSemaphoresInfo.size());
                vkWaitSemaphoreVector.reserve(waitSemaphoresInfo.size());
                AZStd::for_each(waitSemaphoresInfo.begin(), waitSemaphoresInfo.end(), [&](auto& item) 
                { 
                    vkWaitPipelineStages.push_back(item.first);
                    vkWaitSemaphoreVector.push_back(item.second->GetNativeSemaphore());
                    // Wait until the wait semaphores has been submitted for signaling.
                    item.second->WaitEvent();
                });

                submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.pNext = nullptr;
                submitInfo.waitSemaphoreCount = static_cast<uint32_t>(vkWaitSemaphoreVector.size());
                submitInfo.pWaitSemaphores = vkWaitSemaphoreVector.empty() ? nullptr : vkWaitSemaphoreVector.data();
                submitInfo.pWaitDstStageMask = vkWaitPipelineStages.empty() ? nullptr : vkWaitPipelineStages.data();
                submitInfo.commandBufferCount = static_cast<uint32_t>(vkCommandBuffers.size());
                submitInfo.pCommandBuffers = vkCommandBuffers.empty() ? nullptr : vkCommandBuffers.data();
                submitInfo.signalSemaphoreCount = static_cast<uint32_t>(vkSignalSemaphores.size());
                submitInfo.pSignalSemaphores = vkSignalSemaphores.empty() ? nullptr : vkSignalSemaphores.data();
                submitCount++;
            }

            VkFence nativeFence = VK_NULL_HANDLE;
            if (fenceToSignal)
            {
                fenceToSignal->Reset();
                nativeFence = fenceToSignal->GetNativeFence();
            }
            const VkResult result = static_cast<Device&>(GetDevice())
                                        .GetContext()
                                        .QueueSubmit(m_nativeQueue, submitCount, submitCount ? &submitInfo : nullptr, nativeFence);
            AssertSuccess(result);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            // Signal all signaling semaphores that they can be used.
            AZStd::for_each(semaphoresToSignal.begin(), semaphoresToSignal.end(), [&](auto& item) { item->SignalEvent(); });

            if (fenceToSignal)
            {
                fenceToSignal->SignalEvent();
            }
            return RHI::ResultCode::Success;
        }

        void Queue::WaitForIdle()
        {
            if (m_nativeQueue != VK_NULL_HANDLE)
            {
                VkResult result = static_cast<Device&>(GetDevice()).GetContext().QueueWaitIdle(m_nativeQueue);
#if defined(AZ_FORCE_CPU_GPU_INSYNC)
                if (result == VK_ERROR_DEVICE_LOST)
                {
                    AZ_TracePrintf("Device", "The last executing pass before device removal was: %s\n", GetDevice().GetLastExecutingScope().data());
                    GetDevice().SetDeviceRemoved();
                }
#endif
                AssertSuccess(result);
            }
        }

        const Queue::Descriptor& Queue::GetDescriptor() const
        {
            return m_descriptor;
        }

        QueueId Queue::GetId() const
        {
            QueueId id;
            id.m_familyIndex = m_descriptor.m_familyIndex;
            id.m_queueIndex = m_descriptor.m_queueIndex;
            return id;
        }

        void Queue::BeginDebugLabel(const char* label, const AZ::Color color)
        {
            Debug::BeginQueueDebugLabel(
                static_cast<Device&>(m_descriptor.m_commandQueue->GetDevice()).GetContext(), m_nativeQueue, label, color);
        }

        void Queue::EndDebugLabel()
        {
            Debug::EndQueueDebugLabel(static_cast<Device&>(m_descriptor.m_commandQueue->GetDevice()).GetContext(), m_nativeQueue);
        }

        void Queue::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeQueue), name.data(), VK_OBJECT_TYPE_QUEUE, static_cast<Device&>(GetDevice()));
            }
        }
    }
}
