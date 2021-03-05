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

            void UpdateCpuTimingStatistics(RHI::CpuTimingStatistics& cpuTimingStatistics) const;
        private:
            AZStd::array<RHI::Ptr<CommandQueue>, RHI::HardwareQueueClassCount> m_commandQueues;
            FenceSet m_compiledFences;
            AZStd::array<FenceSet, RHI::Limits::Device::FrameCountMax> m_frameFences;
            uint32_t m_currentFrameIndex = 0;
            
            void QueueGpuSignals(FenceSet& fenceSet);
        };
    }
}
