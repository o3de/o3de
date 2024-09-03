/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/WebGPU.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/Device.h>

namespace AZ::WebGPU
{
    void CommandQueueContext::Init(RHI::Device& deviceBase)
    {
        m_currentFrameIndex = 0;

        // WebGPU only supports one queue for now.
        RHI::CommandQueueDescriptor descriptor;
        // WebGPU doesn't support multithreading for now.
        descriptor.m_executePolicy = RHI::CommandQueuePolicy::Serial;
        m_commandQueue = CommandQueue::Create();
        m_commandQueue->Init(deviceBase, descriptor);
    }

    void CommandQueueContext::Shutdown()
    {
        WaitForIdle();
        ForAllQueues(
            [](RHI::Ptr<CommandQueue>& commandQueue)
            {
                commandQueue = nullptr;
            });
    }

    void CommandQueueContext::WaitForIdle()
    {
        AZ_PROFILE_SCOPE(RHI, "CommandQueueContext: WaitForIdle");
        m_commandQueue->WaitForIdle();
    }

    void CommandQueueContext::Begin()
    {
        AZ_PROFILE_SCOPE(RHI, "CommandQueueContext: Begin");

        {
            AZ_PROFILE_SCOPE(RHI, "Clearing Command Queue Timers");
            ForAllQueues(
                [](RHI::Ptr<CommandQueue>& commandQueue)
                {
                    commandQueue->ClearTimers();
                });
        }
    }

    void CommandQueueContext::End()
    {
        AZ_PROFILE_SCOPE(RHI, "CommandQueueContext: End");

        ForAllQueues(
            [](RHI::Ptr<CommandQueue>& commandQueue)
            {
                commandQueue->FlushCommands();
            });

        // Advance to the next frame and wait for its resources to be available before continuing.
        m_currentFrameIndex = (m_currentFrameIndex + 1) % aznumeric_cast<uint32_t>(RHI::Limits::Device::FrameCountMax);
    }

    void CommandQueueContext::ExecuteWork(
        RHI::HardwareQueueClass hardwareQueueClass,
        const ExecuteWorkRequest& request)
    {
        GetCommandQueue(hardwareQueueClass).ExecuteWork(request);
    }

    void CommandQueueContext::ForAllQueues(const AZStd::function<void(RHI::Ptr<CommandQueue>&)>& callback)
    {
        // WebGPU only supports one queue for now.
        callback(m_commandQueue);
    }

    CommandQueue& CommandQueueContext::GetCommandQueue([[maybe_unused]] RHI::HardwareQueueClass hardwareQueueClass)
    {
        return *m_commandQueue;
    }

    const CommandQueue& CommandQueueContext::GetCommandQueue([[maybe_unused]] RHI::HardwareQueueClass hardwareQueueClass) const
    {
        return *m_commandQueue;
    }


}
