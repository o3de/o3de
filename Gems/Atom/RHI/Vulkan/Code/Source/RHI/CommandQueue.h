/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/CommandQueue.h>
#include <Atom/RHI/Object.h>
#include <Atom/RHI/DeviceSwapChain.h>
#include <RHI/CommandList.h>
#include <RHI/Semaphore.h>
#include <RHI/Queue.h>

namespace AZ
{
    namespace Vulkan
    {
        class Fence;

        struct ExecuteWorkRequest
            : public RHI::ExecuteWorkRequest
        {
            /// Primary command buffer to queue
            RHI::Ptr<CommandList> m_commandList;

            /// Set of semaphores to wait before execution of commands
            AZStd::vector<Semaphore::WaitSemaphore> m_semaphoresToWait;

            /// Set of semaphores to signal after execution of commands
            AZStd::vector<RHI::Ptr<Semaphore>> m_semaphoresToSignal;

            /// Fence to signal after execution of commands
            AZStd::vector<RHI::Ptr<Fence>> m_fencesToSignal;

            /// Fence to wait for before execution of command
            AZStd::vector<RHI::Ptr<Fence>> m_fencesToWaitFor;

            /// Debug label to insert during work execution
            AZStd::string m_debugLabel;
        };

        struct CommandQueueDescriptor
            : public RHI::CommandQueueDescriptor
        {
            uint32_t m_queueIndex = 0;
        };

        class CommandQueue final
            : public RHI::CommandQueue
        {
            using Base = RHI::Object;

        public:
            AZ_CLASS_ALLOCATOR(CommandQueue, AZ::SystemAllocator);
            AZ_RTTI(CommandQueue, "7C97F9F7-C582-4575-8A6B-A7778821AF33", Base);

            static RHI::Ptr<CommandQueue> Create();

            //////////////////////////////////////////////////////////////////////////
            // RHI::CommandQueue
            void ExecuteWork(const RHI::ExecuteWorkRequest& request) override;
            void WaitForIdle() override;
            //////////////////////////////////////////////////////////////////////////

            const Queue::Descriptor& GetQueueDescriptor() const;
            
            void Signal(Fence& fence);
            VkPipelineStageFlags GetSupportedPipelineStages() const;

            QueueId GetId() const;

            void ClearTimers();

            AZStd::sys_time_t GetLastExecuteDuration() const;
            AZStd::sys_time_t GetLastPresentDuration() const;

        private:
            CommandQueue() = default;
            ~CommandQueue() = default;           

            //////////////////////////////////////////////////////////////////////////
            // RHI::CommandQueue
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::CommandQueueDescriptor& descriptor) override;
            void ShutdownInternal() override;
            void* GetNativeQueue() override;
            //////////////////////////////////////////////////////////////////////////

            VkPipelineStageFlags CalculateSupportedPipelineStages() const;

            Queue::Descriptor m_queueDescriptor;
            RHI::Ptr<Queue> m_queue;

            VkPipelineStageFlags m_supportedStagesMask = 0;

            AZStd::sys_time_t m_lastExecuteDuration{};
            AZStd::sys_time_t m_lastPresentDuration{};
        };
    }
}
