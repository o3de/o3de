/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Object.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <RHI/CommandQueue.h>

namespace AZ
{
    namespace DX12
    {
        class CommandQueueContext
        {
        public:
            AZ_CLASS_ALLOCATOR(CommandQueueContext, AZ::SystemAllocator, 0);

            CommandQueueContext() = default;

            void Init(RHI::Device& deviceBase);

            void Shutdown();

            CommandQueue& GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass);
            const CommandQueue& GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass) const;

            void Begin();

            uint64_t IncrementFence(RHI::HardwareQueueClass hardwareQueueClass);

            void QueueGpuSignals(FenceSet& fenceSet);

            void WaitForIdle();

            void End();

            void ExecuteWork(
                RHI::HardwareQueueClass hardwareQueueClass,
                const ExecuteWorkRequest& request);

            void UpdateCpuTimingStatistics() const;
            
            // Fences across all queues that are compiled by the frame graph compilation phase
            const FenceSet& GetCompiledFences();
            
        private:
            void CalibrateClocks();

            AZStd::array<RHI::Ptr<CommandQueue>, RHI::HardwareQueueClassCount> m_commandQueues;

            FenceSet m_compiledFences;
            AZStd::vector<FenceSet> m_frameFences;
            uint32_t m_currentFrameIndex = 0;
        };
    }
}
