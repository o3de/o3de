/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <RHI/CommandQueue.h>
#include <RHI/SwapChain.h>
#include <AzCore/Debug/Timer.h>

namespace AZ::WebGPU
{
    RHI::Ptr<CommandQueue> CommandQueue::Create()
    {
        return aznew CommandQueue();
    }

    RHI::ResultCode CommandQueue::InitInternal(RHI::Device& deviceBase, [[maybe_unused]] const RHI::CommandQueueDescriptor& descriptor)
    {
        Device& device = static_cast<Device&>(deviceBase);
        m_wgpuQueue = device.GetNativeDevice().GetQueue();
        AZ_Assert(m_wgpuQueue, "Invalid queue");
        return m_wgpuQueue ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

    void CommandQueue::ShutdownInternal()
    {
        m_wgpuQueue = nullptr;
    }

    void* CommandQueue::GetNativeQueue()
    {
        return &m_wgpuQueue;
    }

    void CommandQueue::ExecuteWork(const RHI::ExecuteWorkRequest& rhiRequest)
    {
        const ExecuteWorkRequest& request = static_cast<const ExecuteWorkRequest&>(rhiRequest);
        QueueCommand(
            [=]([[maybe_unused]] void* commandQueue)
            {
                AZ_PROFILE_SCOPE(RHI, "ExecuteWork");
                AZ::Debug::ScopedTimer executionTimer(m_lastExecuteDuration);
                AZStd::vector<wgpu::CommandBuffer> commandBuffers;
                commandBuffers.reserve(request.m_commandLists.size());
                for (CommandList* commandList : request.m_commandLists)
                {
                    const auto& wgpuCommandBuffer = commandList->GetNativeCommandBuffer();
                    if (wgpuCommandBuffer)
                    {
                        commandBuffers.push_back(AZStd::move(wgpuCommandBuffer));
                    }
                }

                if (!commandBuffers.empty())
                {
                    m_wgpuQueue.Submit(commandBuffers.size(), commandBuffers.data());
                }

                AZ::Debug::ScopedTimer presentTimer(m_lastPresentDuration);
                for (RHI::DeviceSwapChain* swapChain : request.m_swapChainsToPresent)
                {
                    swapChain->Present();
                }
            });
    }

    void CommandQueue::WaitForIdle()
    {
    }

    void CommandQueue::ClearTimers()
    {
        m_lastExecuteDuration = 0;
    }

    void CommandQueue::WriteBuffer(const Buffer& buffer, uint64_t bufferOffset, AZStd::span<const uint8_t> data)
    {
        QueueCommand(
            [&buffer, bufferOffset, data, this]([[maybe_unused]] void* commandQueue)
            {
                m_wgpuQueue.WriteBuffer(buffer.GetNativeBuffer(), bufferOffset, data.data(), data.size());
            });
    }
}
