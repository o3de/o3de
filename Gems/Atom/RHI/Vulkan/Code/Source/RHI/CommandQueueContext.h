/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <RHI/CommandQueue.h>
#include <RHI/Fence.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class SwapChain;

        class CommandQueueContext final
        {
        public:
            AZ_CLASS_ALLOCATOR(CommandQueueContext, AZ::SystemAllocator);

            CommandQueueContext(const CommandQueueContext&) = delete;
            CommandQueueContext& operator=(const CommandQueueContext&) = delete;
            CommandQueueContext(CommandQueueContext&&) = delete;
            CommandQueueContext& operator=(CommandQueueContext&&) = delete;

            struct Descriptor
            {
                uint32_t m_frameCountMax = RHI::Limits::Device::FrameCountMax;
            };

            CommandQueueContext() = default;
            ~CommandQueueContext() = default;

            RHI::ResultCode Init(RHI::Device& device, Descriptor& descriptor);

            void Begin();
            void End();

            void Shutdown();
            void WaitForIdle();

            CommandQueue& GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass) const;
            CommandQueue& GetOrCreatePresentationCommandQueue(const SwapChain& swapchain);
            CommandQueue& GetPresentationCommandQueue() const;
            RHI::Ptr<Fence> GetFrameFence(RHI::HardwareQueueClass hardwareQueueClass) const;
            RHI::Ptr<Fence> GetFrameFence(const QueueId& queueId) const;
            uint32_t GetCurrentFrameIndex() const;
            uint32_t GetFrameCount() const;
            uint32_t GetQueueFamilyIndex(const RHI::HardwareQueueClass hardwareQueueClass) const;
            AZStd::vector<uint32_t> GetQueueFamilyIndices(const RHI::HardwareQueueClassMask hardwareQueueClassMask) const;
            VkPipelineStageFlags GetSupportedPipelineStages(uint32_t queueFamilyIndex) const;

            void UpdateCpuTimingStatistics() const;

        private:
            Descriptor m_descriptor;

            RHI::ResultCode BuildQueues(Device& device);
            RHI::ResultCode CreateQueue(Device& device, uint32_t& commandQueueIndex, uint32_t familyIndex, const char* name);

            bool SelectQueueFamily(
                Device& device,
                uint32_t& result,
                const RHI::HardwareQueueClassMask queueMask,
                AZStd::function<bool(int32_t)> reqFunction = nullptr);

            AZStd::vector<RHI::Ptr<CommandQueue>> m_commandQueues;
            /// Maps queue classes to the index in the m_commandQueues vector.
            AZStd::array<uint32_t, RHI::HardwareQueueClassCount> m_queueClassMapping;

            using FencesPerQueue = AZStd::vector<RHI::Ptr<Fence>>;
            AZStd::array<FencesPerQueue, RHI::Limits::Device::FrameCountMax> m_frameFences;
            uint32_t m_currentFrameIndex = 0;

            /// Number of created queues per each of the queue families
            AZStd::vector<uint32_t> m_numCreatedQueuesPerFamily;

            // Index to the presentation queue
            const uint32_t InvalidIndex = aznumeric_cast<uint32_t>(-1);
            uint32_t m_presentationQueueIndex = InvalidIndex;
        };
    }
}
