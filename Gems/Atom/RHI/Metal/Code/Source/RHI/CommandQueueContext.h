/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/CommandQueue.h>

namespace AZ
{
    namespace Metal
    {
        class CommandQueueContext
        {
        public:
            AZ_CLASS_ALLOCATOR(CommandQueueContext, AZ::SystemAllocator, 0);
            
            CommandQueueContext() = default;
            void Init(RHI::Device& device);
            void Shutdown();

            CommandQueue& GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass);
            const CommandQueue& GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass) const;

            void Begin();

            uint64_t IncrementHWQueueFence(RHI::HardwareQueueClass hardwareQueueClass);
            
            void WaitForIdle();

            void End();

            void ExecuteWork(
                RHI::HardwareQueueClass hardwareQueueClass,
                const ExecuteWorkRequest& request);
            
            /// Fences across all queues that are compiled by the frame graph compilation phase
            const FenceSet& GetCompiledFences();

            void UpdateCpuTimingStatistics() const;
        private:
            AZStd::array<RHI::Ptr<CommandQueue>, RHI::HardwareQueueClassCount> m_commandQueues;
            FenceSet m_compiledFences;
            AZStd::array<FenceSet, RHI::Limits::Device::FrameCountMax> m_frameFences;
            uint32_t m_currentFrameIndex = 0;
            
            void QueueGpuSignals(FenceSet& fenceSet);
        };
    }
}
