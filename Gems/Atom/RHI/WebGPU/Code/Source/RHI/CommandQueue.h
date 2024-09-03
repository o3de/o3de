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


namespace AZ::WebGPU
{
    struct ExecuteWorkRequest : public RHI::ExecuteWorkRequest
    {
        /// Command lists to queue.
        AZStd::vector<CommandList*> m_commandLists;
    };

    //! Implementation of the command queue for WebGPU
    class CommandQueue final
        : public RHI::CommandQueue
    {
        using Base = RHI::CommandQueue;
    public:
        AZ_CLASS_ALLOCATOR(CommandQueue, AZ::SystemAllocator);
        AZ_RTTI(CommandQueue, "{FF877C27-FC9D-4321-B8AA-30D4EC0C2451}", Base);
            
        using Descriptor = RHI::CommandQueueDescriptor;
        static RHI::Ptr<CommandQueue> Create();
        
        //////////////////////////////////////////////////////////////////////////
        // RHI::CommandQueue
        void ExecuteWork(const RHI::ExecuteWorkRequest& request) override;
        void WaitForIdle() override;
        //////////////////////////////////////////////////////////////////////////

        //! Reset timers for measuring duration of execution and presentation
        void ClearTimers();

    private:
        CommandQueue() = default;

        //////////////////////////////////////////////////////////////////////////
        // RHI::CommandQueue
        RHI::ResultCode InitInternal(RHI::Device& device, const RHI::CommandQueueDescriptor& descriptor) override;
        void ShutdownInternal() override;
        void* GetNativeQueue() override;
        //////////////////////////////////////////////////////////////////////////

        //! Native queue
        wgpu::Queue m_wgpuQueue = nullptr;

        //! Used for measuring cpu time for execution duration
        AZStd::sys_time_t m_lastExecuteDuration{};
        //! Used for measuring cpu time for presentation duration
        AZStd::sys_time_t m_lastPresentDuration{};
    };
}
