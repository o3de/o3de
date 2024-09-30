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

namespace AZ::WebGPU
{
    class Fence;

    //! Provides centralize access to the Command queues provided by the implementation 
    class CommandQueueContext final
    {
    public:
        AZ_CLASS_ALLOCATOR(CommandQueueContext, AZ::SystemAllocator);

        CommandQueueContext(const CommandQueueContext&) = delete;
        CommandQueueContext& operator=(const CommandQueueContext&) = delete;
        CommandQueueContext(CommandQueueContext&&) = delete;
        CommandQueueContext& operator=(CommandQueueContext&&) = delete;

        CommandQueueContext() = default;
        ~CommandQueueContext() = default;

        struct Descriptor
        {
            uint32_t m_frameCountMax = RHI::Limits::Device::FrameCountMax;
        };

        RHI::ResultCode Init(RHI::Device& deviceBase, const Descriptor& descriptor);

        void Shutdown();

        CommandQueue& GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass);
        const CommandQueue& GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass) const;

        void Begin();

        //! Wait until all gpu work has finished on all available queues
        void WaitForIdle();

        void End();

        //! Execute gpu work in the proper queue
        void ExecuteWork(
            RHI::HardwareQueueClass hardwareQueueClass,
            const ExecuteWorkRequest& request);

        RHI::Ptr<Fence> GetFrameFence(RHI::HardwareQueueClass hardwareQueueClass) const;
        uint32_t GetFrameCount() const;
            
    private:        
        void ForAllQueues(const AZStd::function<void(RHI::Ptr<CommandQueue>&)>& callback);

        Descriptor m_descriptor;

        //! Single command queue since WebGPU only supports one at the moment
        RHI::Ptr<CommandQueue> m_commandQueue;
        AZStd::array<RHI::Ptr<Fence>, RHI::Limits::Device::FrameCountMax> m_frameFences;
        uint32_t m_currentFrameIndex = 0;
    };
}
