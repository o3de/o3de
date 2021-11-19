/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
#include <AzCore/Debug/EventTrace.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/Device.h>
#include <RHI/Semaphore.h>
#include <RHI/Conversion.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode CommandQueueContext::Init(RHI::Device& device, Descriptor& descriptor)
        {
            m_descriptor = descriptor;
            Device& vulkanDevice = static_cast<Device&>(device);

            m_numCreatedQueuesPerFamily.resize(vulkanDevice.GetQueueFamilyProperties().size(), 0);
            RHI::ResultCode result = BuildQueues(vulkanDevice);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            return result;
        }

        void CommandQueueContext::Begin()
        {
            for (const RHI::Ptr<CommandQueue>& commandQueue : m_commandQueues)
            {
                commandQueue->ClearTimers();
            }
        }

        void CommandQueueContext::End()
        {
            AZ_PROFILE_SCOPE(RHI, "CommandQueueContext: End");

            for (auto& commandQueue : m_commandQueues)
            {
                commandQueue->Signal(*GetFrameFence(commandQueue->GetId()));
                commandQueue->FlushCommands();
            }

            // Advance to the next frame and wait for its resources to be available before continuing.
            m_currentFrameIndex = (m_currentFrameIndex + 1) % GetFrameCount();

            {
                AZ_PROFILE_SCOPE(RHI, "Wait on Fences");

                FencesPerQueue& nextFences = m_frameFences[m_currentFrameIndex];
                for (auto& fence : nextFences)
                {
                    fence->WaitOnCpu();
                }
            }
        }

        void CommandQueueContext::Shutdown()
        {
            WaitForIdle();

            for (FencesPerQueue& queueFrameFences : m_frameFences)
            {
                queueFrameFences.clear();
            }

            m_commandQueues.clear();
        }

        void CommandQueueContext::WaitForIdle()
        {
            AZ_PROFILE_SCOPE(RHI, "CommandQueueContext: WaitForIdle");
            for (auto& commandQueue : m_commandQueues)
            {
                commandQueue->WaitForIdle();
            }
        }

        CommandQueue& CommandQueueContext::GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass) const
        {
            uint32_t commandQueueIndex = m_queueClassMapping[static_cast<uint32_t>(hardwareQueueClass)];
            return *(m_commandQueues[commandQueueIndex]);
        }

        CommandQueue& CommandQueueContext::GetOrCreatePresentationCommandQueue(const SwapChain& swapchain)
        {
            auto& device = static_cast<Device&>(swapchain.GetDevice());
            // First search among the existing queues if they support presentation for the format of the swapchain
            VkPhysicalDevice vkPhysicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice()).GetNativePhysicalDevice();
            auto supportsPresentation = [&swapchain, &vkPhysicalDevice](uint32_t familyIndex)
            {
                VkBool32 supported = VK_FALSE;
                AssertSuccess(vkGetPhysicalDeviceSurfaceSupportKHR(
                    vkPhysicalDevice,
                    familyIndex,
                    swapchain.GetSurface().GetNativeSurface(),
                    &supported));
                return supported == VK_TRUE;
            };

            for (const auto& commandQueue : m_commandQueues)
            {
                if (supportsPresentation(commandQueue->GetQueueDescriptor().m_familyIndex))
                {
                    return *commandQueue;
                }
            }

            // No luck, we need to create a new queue.
            uint32_t familyIndex = 0;
            uint32_t commandQueueIndex = 0;
            if (SelectQueueFamily(device, familyIndex, RHI::HardwareQueueClassMask::None, supportsPresentation))
            {                
                [[maybe_unused]] RHI::ResultCode result = CreateQueue(device, commandQueueIndex, familyIndex, "Presentation queue");
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to create presentation queue");                
            }
            else
            {
                AZ_Assert(false, "Failed to find a queue suitable for presentation");
            }          
            return *m_commandQueues[commandQueueIndex];
        }

        RHI::Ptr<Fence> CommandQueueContext::GetFrameFence(RHI::HardwareQueueClass hardwareQueueClass) const
        {
            uint32_t commandQueueIndex = m_queueClassMapping[static_cast<uint32_t>(hardwareQueueClass)];
            return m_frameFences[m_currentFrameIndex][commandQueueIndex];
        }

        RHI::Ptr<Fence> CommandQueueContext::GetFrameFence(const QueueId& queueId) const
        {
            for (uint32_t i = 0; i < m_commandQueues.size(); ++i)
            {
                if (m_commandQueues[i]->GetId() == queueId)
                {
                    return m_frameFences[m_currentFrameIndex][i];
                }
            }
            AZ_Assert(false, "Could not find frame fence for queueId=(%lu, %lu)", static_cast<unsigned long>(queueId.m_familyIndex), static_cast<unsigned long>(queueId.m_queueIndex));
            return RHI::Ptr<Fence>();
        }

        uint32_t CommandQueueContext::GetCurrentFrameIndex() const
        {
            return m_currentFrameIndex;
        }

        uint32_t CommandQueueContext::GetFrameCount() const
        {
            return m_descriptor.m_frameCountMax;
        }

        uint32_t CommandQueueContext::GetQueueFamilyIndex(const RHI::HardwareQueueClass hardwareQueueClass) const
        {
            return GetCommandQueue(hardwareQueueClass).GetQueueDescriptor().m_familyIndex;
        }

        AZStd::vector<uint32_t> CommandQueueContext::GetQueueFamilyIndices(const RHI::HardwareQueueClassMask hardwareQueueClassMask) const
        {
            AZStd::vector<uint32_t> queueFamilies;
            for (uint32_t i = 0; i < RHI::HardwareQueueClassCount; ++i)
            {
                if (RHI::CheckBitsAny(hardwareQueueClassMask, static_cast<RHI::HardwareQueueClassMask>(AZ_BIT(i))))
                {
                    uint32_t familyIndex = GetQueueFamilyIndex(static_cast<RHI::HardwareQueueClass>(i));
                    queueFamilies.push_back(familyIndex);
                }
            }

            // Remove duplicates
            AZStd::sort(queueFamilies.begin(), queueFamilies.end());
            queueFamilies.erase(AZStd::unique(queueFamilies.begin(), queueFamilies.end()), queueFamilies.end());
            return queueFamilies;
        }

        VkPipelineStageFlags CommandQueueContext::GetSupportedPipelineStages(uint32_t queueFamilyIndex) const
        {
            for (const auto& commandQueue : m_commandQueues)
            {
                if (commandQueue->GetId().m_familyIndex == queueFamilyIndex)
                {
                    // All members of a family have the same properties.
                    return commandQueue->GetSupportedPipelineStages();
                }
            }

            AZ_Assert(false, "Failed to find queue for family %d", queueFamilyIndex);
            return VkPipelineStageFlags();
        }

        RHI::ResultCode CommandQueueContext::BuildQueues(Device& device)
        {
            static const char* QueueNames[RHI::HardwareQueueClassCount] =
            {
                "Graphics Submit Queue",
                "Compute Submit Queue",
                "Copy Submit Queue"
            };

            // List of queue flags needed per each Hardware Class. Each of those lists is order by priority.
            const AZStd::vector<RHI::HardwareQueueClassMask> queueFlags[RHI::HardwareQueueClassCount] =
            {
                // For the RHI we want a graphic queue that is also able to do compute and copy. According to the standard there has to be
                // at least one queue that supports graphic, compute and copy.
                // Also according to the standard, Graphic and Compute queues can also do Copy but sometimes don't expose the Copy flag.
                { RHI::HardwareQueueClassMask::All , RHI::HardwareQueueClassMask::Graphics | RHI::HardwareQueueClassMask::Compute },
                { RHI::HardwareQueueClassMask::Compute | RHI::HardwareQueueClassMask::Copy, RHI::HardwareQueueClassMask::Compute },
                // We first try to look for a queue that does only Copy and if that fails, we just look for a normal
                // Compute or Graphic queue.
                { RHI::HardwareQueueClassMask::Copy, RHI::HardwareQueueClassMask::Compute, RHI::HardwareQueueClassMask::Graphics }
            };
            
            RHI::ResultCode result;
            for (uint32_t hardwareQueueIndex = 0; hardwareQueueIndex < RHI::HardwareQueueClassCount; ++hardwareQueueIndex)
            {
                // Find a queue family that supports the desired queue.
                for (const RHI::HardwareQueueClassMask mask : queueFlags[hardwareQueueIndex])
                {
                    uint32_t selectedFamilyQueue = 0;
                    if (SelectQueueFamily(device, selectedFamilyQueue, mask))
                    {
                        uint32_t commandQueueIndex = 0;
                        result = CreateQueue(device, commandQueueIndex, selectedFamilyQueue, QueueNames[hardwareQueueIndex]);
                        RETURN_RESULT_IF_UNSUCCESSFUL(result);
                        m_queueClassMapping[hardwareQueueIndex] = static_cast<uint32_t>(commandQueueIndex);
                        break;
                    }
                }                
            }

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode CommandQueueContext::CreateQueue(Device& device, uint32_t& commandQueueIndex, uint32_t familyIndex, const char* name)
        {
            const auto& queueFamilyProperties = device.GetQueueFamilyProperties();
            // Check if we already achieved the max number of queues for this family.
            if (m_numCreatedQueuesPerFamily[familyIndex] >= queueFamilyProperties[familyIndex].queueCount)
            {
                // We need to reuse a previously created queue.
                uint32_t newQueueIndex = (m_numCreatedQueuesPerFamily[familyIndex] % queueFamilyProperties[familyIndex].queueCount);
                // Find the command queue with the provided family and queue indices.
                auto findIt = AZStd::find_if(m_commandQueues.begin(), m_commandQueues.end(), [&familyIndex, &newQueueIndex](const RHI::Ptr<CommandQueue>& commandQueue)
                {
                    const auto& descriptor = commandQueue->GetQueueDescriptor();
                    return descriptor.m_familyIndex == familyIndex && descriptor.m_queueIndex == newQueueIndex;
                });

                if (findIt != m_commandQueues.end())
                {
                    commandQueueIndex = static_cast<uint32_t>(AZStd::distance(m_commandQueues.begin(), findIt));
                    AZStd::string newName = AZStd::string::format("%s, %s", m_commandQueues[commandQueueIndex]->GetName().GetCStr(), name);
                    m_commandQueues[commandQueueIndex]->SetName(Name{ newName });
                    ++m_numCreatedQueuesPerFamily[familyIndex];
                    return RHI::ResultCode::Success;
                }
            }

            CommandQueueDescriptor commandQueueDesc;
            commandQueueDesc.m_queueIndex = m_numCreatedQueuesPerFamily[familyIndex];
            commandQueueDesc.m_hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(familyIndex);
            auto commandQueue = CommandQueue::Create();

            // Set the thread name first for its thread
            commandQueue->SetName(Name{ name });

            RHI::ResultCode result = commandQueue->Init(device, commandQueueDesc);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            m_commandQueues.push_back(commandQueue);
            commandQueueIndex = static_cast<uint32_t>(m_commandQueues.size() - 1);
            ++m_numCreatedQueuesPerFamily[familyIndex];

            // Build frame fences for queue
            AZ_Assert(GetFrameCount() <= m_frameFences.size(), "FrameCount is too large.");
            for (uint32_t index = 0; index < GetFrameCount(); ++index)
            {
                RHI::Ptr<Fence> fence = Fence::Create();
                result = fence->Init(device, RHI::FenceState::Signaled);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
                m_frameFences[index].push_back(fence);
            }
            return result;
        }

        bool CommandQueueContext::SelectQueueFamily(
            Device& device,
            uint32_t& selectedQueueFamilyIndex,
            const RHI::HardwareQueueClassMask queueMask,
            AZStd::function<bool(int32_t)> reqFunction)
        {
            const uint32_t InvalidFamilyIndex = static_cast<uint32_t>(-1);
            struct QueueSelection
            {
                uint32_t m_familyIndex = InvalidFamilyIndex;
                bool m_newQueue = false;
                uint32_t m_remainingFlags = std::numeric_limits<uint32_t>::max();

                bool operator>(const QueueSelection& other) const
                {
                    // Prefer the queue selection that creates a new queue instead of reusing.
                    if (m_newQueue != other.m_newQueue)
                    {
                        return m_newQueue;
                    }

                    // Prefer the queue selection that is a "better" match from a flags perspective (i.e. less unmatched queue flags).
                    // Better match generally means preferring "dedicated" queues. For example, we prefer a dedicated Copy queue (instead of a graphic one),
                    // because it's better suited to do transfer operations.
                    return RHI::CountBitsSet(m_remainingFlags) < RHI::CountBitsSet(other.m_remainingFlags);
                }
            };

            VkQueueFlags flags = 0;
            for (uint32_t i = 0; i < RHI::HardwareQueueClassCount; ++i)
            {
                if (RHI::CheckBitsAll(queueMask, static_cast<RHI::HardwareQueueClassMask>(AZ_BIT(i))))
                {
                    flags |= ConvertQueueClass(static_cast<RHI::HardwareQueueClass>(i));
                }
            }

            const auto& queueFamilyProperties = device.GetQueueFamilyProperties();
            QueueSelection queueSelection;
            for (uint32_t familyIndex = 0; familyIndex < queueFamilyProperties.size(); ++familyIndex)
            {
                // Check any special requirements
                if (reqFunction && !reqFunction(familyIndex))
                {
                    continue;
                }

                QueueSelection newSelection;
                const VkQueueFamilyProperties& queueProperties = queueFamilyProperties[familyIndex];
                // First check if it has the necessary flags.
                if (!RHI::CheckBitsAll(queueProperties.queueFlags, flags))
                {
                    continue;
                }

                newSelection.m_familyIndex = familyIndex;
                // Calculate the flags that are not matched with the ones we are looking for.
                // We use it to compare which selection is better.
                newSelection.m_remainingFlags = RHI::ResetBits(queueProperties.queueFlags, flags);
                newSelection.m_newQueue = m_numCreatedQueuesPerFamily[familyIndex] < queueProperties.queueCount;

                // Check if this queue is a better selection.
                if (newSelection > queueSelection)
                {
                    queueSelection = newSelection;
                }
            }

            selectedQueueFamilyIndex = queueSelection.m_familyIndex;
            return queueSelection.m_familyIndex != InvalidFamilyIndex;
        }

        void CommandQueueContext::UpdateCpuTimingStatistics() const
        {
            if (auto statsProfiler = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get(); statsProfiler)
            {
                auto& rhiMetrics = statsProfiler->GetProfiler(rhiMetricsId);

                AZStd::sys_time_t presentDuration = 0;
                for (const RHI::Ptr<CommandQueue>& commandQueue : m_commandQueues)
                {
                    const AZ::Crc32 commandQueueId(commandQueue->GetName().GetHash());
                    rhiMetrics.PushSample(commandQueueId, static_cast<double>(commandQueue->GetLastExecuteDuration()));
                    presentDuration += commandQueue->GetLastPresentDuration();
                }

                rhiMetrics.PushSample(AZ_CRC_CE("Present"), static_cast<double>(presentDuration));
            }
        }
    }
}
