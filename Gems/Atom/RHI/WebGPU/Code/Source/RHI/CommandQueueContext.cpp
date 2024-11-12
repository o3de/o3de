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
#include <RHI/Fence.h>

namespace AZ::WebGPU
{
    RHI::ResultCode CommandQueueContext::Init(RHI::Device& deviceBase, const Descriptor& descriptor)
    {
        m_descriptor = descriptor;
        m_currentFrameIndex = 0;

        // WebGPU only supports one queue for now.
        RHI::CommandQueueDescriptor commandQueueDescriptor;
        // WebGPU doesn't support multithreading for now.
        commandQueueDescriptor.m_executePolicy = RHI::CommandQueuePolicy::Serial;
        m_commandQueue = CommandQueue::Create();
        RHI::ResultCode result = m_commandQueue->Init(deviceBase, commandQueueDescriptor);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);
        // Build frame fences for queue
        AZ_Assert(GetFrameCount() <= m_frameFences.size(), "FrameCount is too large.");
        for (uint32_t index = 0; index < GetFrameCount(); ++index)
        {
            RHI::Ptr<Fence> fence = Fence::Create();
            result = fence->Init(deviceBase, RHI::FenceState::Signaled);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            m_frameFences[index] = fence;
        }
        return result;
    }

    void CommandQueueContext::Shutdown()
    {
        WaitForIdle();
        m_frameFences.fill(nullptr);
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
            [this](RHI::Ptr<CommandQueue>& commandQueue)
            {
                commandQueue->Signal(*GetFrameFence(commandQueue->GetHardwareQueueClass()));
                commandQueue->FlushCommands();
            });

        // Advance to the next frame and wait for its resources to be available before continuing.
        m_currentFrameIndex = (m_currentFrameIndex + 1) % aznumeric_cast<uint32_t>(RHI::Limits::Device::FrameCountMax);

        {
            AZ_PROFILE_SCOPE(RHI, "Wait on Fences");

            m_frameFences[m_currentFrameIndex]->WaitOnCpu();
        }
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

    RHI::Ptr<Fence> CommandQueueContext::GetFrameFence([[maybe_unused]] RHI::HardwareQueueClass hardwareQueueClass) const
    {
        return m_frameFences[m_currentFrameIndex];
    }

    uint32_t CommandQueueContext::GetFrameCount() const
    {
        return m_descriptor.m_frameCountMax;
    }
}
