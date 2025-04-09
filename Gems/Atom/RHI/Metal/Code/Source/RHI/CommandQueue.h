/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/CommandQueue.h>
#include <RHI/CommandList.h>
#include <RHI/CommandQueueCommandBuffer.h>
#include <RHI/Fence.h>
#include <RHI/Metal.h>

namespace AZ
{
    namespace Metal
    {
        struct ExecuteWorkRequest
            : public RHI::ExecuteWorkRequest
        {
            /// Command lists to queue.
            AZStd::vector<CommandList*> m_commandLists;

            /// A set of scope fences to signal after executing the command lists.
            AZStd::vector<Fence*> m_scopeFencesToSignal;
            
            static constexpr uint64_t FenceValueNull = 0;

            //! Metal command buffer associated with this work request
            CommandQueueCommandBuffer* m_commandBuffer = nullptr;
            
            /// A set of fence values for each queue class to wait on
            /// before execution. Will ignore if null.
            FenceValueSet m_waitFenceValues = { { FenceValueNull } };

            /// A fence value to signal after execution. Ignored if null.
            uint64_t m_signalFenceValue = FenceValueNull;
        };
        
        class CommandQueue final
            : public RHI::CommandQueue
        {
            using Base = RHI::CommandQueue;
        public:
            AZ_CLASS_ALLOCATOR(CommandQueue, AZ::SystemAllocator);
            AZ_RTTI(CommandQueue, "{C50C1546-EC3B-45A3-BF48-C2A99C1BAE8A}", Base);
            
            using Descriptor = RHI::CommandQueueDescriptor;
            static RHI::Ptr<CommandQueue> Create();
        
            id<MTLCommandQueue> GetPlatformQueue() const;
            CommandQueueCommandBuffer& GetCommandBuffer();
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::CommandQueue
            void ExecuteWork(const RHI::ExecuteWorkRequest& request) override;
            void WaitForIdle() override;
            //////////////////////////////////////////////////////////////////////////

            void QueueGpuSignal(Fence& fence);
            void QueueGpuSignal(Fence& fence, uint64_t signalValue);
            
            void ClearTimers();

            AZStd::sys_time_t GetLastExecuteDuration() const;
            AZStd::sys_time_t GetLastPresentDuration() const;
        private:
            CommandQueue() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::CommandQueue
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::CommandQueueDescriptor& descriptor) override;
            void ShutdownInternal() override;
            void* GetNativeQueue() override;
            //////////////////////////////////////////////////////////////////////////

            
            void ProcessQueue();
            
            CommandQueueCommandBuffer m_commandBuffer;
            id<MTLCommandQueue> m_hwQueue = nil;
            
            AZStd::sys_time_t m_lastExecuteDuration{};
            AZStd::sys_time_t m_lastPresentDuration{};
        };
    }
}
