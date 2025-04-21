/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/CommandQueue.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>
#include <RHI/SwapChain.h>
#include <RHI/Conversions.h>

#include <AzCore/Debug/Timer.h>

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            HRESULT CreateCommandQueue(ID3D12DeviceX* device, RHI::HardwareQueueClass hardwareQueueClass, HardwareQueueSubclass hardwareQueueSubclass, ID3D12CommandQueueX** commandQueue);
        }

        RHI::Ptr<CommandQueue> CommandQueue::Create()
        {
            return aznew CommandQueue();
        }

        ID3D12CommandQueue* CommandQueue::GetPlatformQueue() const
        {
            return m_queue.get();
        }

        const char* GetQueueName(RHI::HardwareQueueClass hardwareQueueClass, HardwareQueueSubclass hardwareQueueSubclass)
        {
            switch (hardwareQueueClass)
            {
            case RHI::HardwareQueueClass::Copy:
                switch (hardwareQueueSubclass)
                {
                case HardwareQueueSubclass::Primary:
                    return "Copy Queue (Primary)";
                case HardwareQueueSubclass::Secondary:
                    return "Copy Queue (Secondary)";
                default:
                    AZ_Assert(false, "Unknown Hardware Queue Subclass");
                    return "Copy Queue";
                }

            case RHI::HardwareQueueClass::Compute:
                switch (hardwareQueueSubclass)
                {
                case HardwareQueueSubclass::Primary:
                    return "Compute Queue (Primary)";
                case HardwareQueueSubclass::Secondary:
                    return "Compute Queue (Secondary)";
                default:
                    AZ_Assert(false, "Unknown Hardware Queue Subclass");
                    return "Compute Queue";
                }

            case RHI::HardwareQueueClass::Graphics:
                return "Graphics Queue";

            default:
                AZ_Assert(false, "Unknown Hardware Queue");
                return "Unknown Queue";
            }
        }

        RHI::ResultCode CommandQueue::InitInternal(RHI::Device& deviceBase, const RHI::CommandQueueDescriptor& descriptor)
        {
            DeviceObject::Init(deviceBase);
            Microsoft::WRL::ComPtr<ID3D12CommandQueueX> queue = nullptr;

            ID3D12DeviceX* device = static_cast<Device&>(deviceBase).GetDevice();
            const CommandQueueDescriptor& commandQueueDesc = static_cast<const CommandQueueDescriptor&>(descriptor);
            DX12::AssertSuccess(Platform::CreateCommandQueue(device, descriptor.m_hardwareQueueClass, commandQueueDesc.m_hardwareQueueSubclass, queue.GetAddressOf()));

            const char* queueName = GetQueueName(commandQueueDesc.m_hardwareQueueClass, commandQueueDesc.m_hardwareQueueSubclass);

            AZ_Assert(queueName, "Incorrectly handled HardwareQueueClass");
            if (queueName)
            {
                AZStd::wstring queueNameW;
                AZStd::to_wstring(queueNameW, queueName);

                queue->SetName(queueNameW.c_str());

                SetName(Name{ queueName });
            }

            m_queue = queue.Get();
            m_device = device;
            m_hardwareQueueSubclass = commandQueueDesc.m_hardwareQueueSubclass;
            return RHI::ResultCode::Success;
        }

        void CommandQueue::ShutdownInternal()
        {
            if (m_queue)
            {
                m_queue.reset();
            }

        }

        void CommandQueue::QueueGpuSignal(Fence& fence)
        {
            QueueCommand([&fence](void* commandQueue)
            {
                AZ_PROFILE_SCOPE(RHI, "SignalFence");
                ID3D12CommandQueue* dx12CommandQueue = static_cast<ID3D12CommandQueue*>(commandQueue);
                dx12CommandQueue->Signal(fence.Get(), fence.GetPendingValue());
            });
        }

        void CommandQueue::CalibrateClock()
        {
            if (GetDescriptor().m_hardwareQueueClass != RHI::HardwareQueueClass::Copy)
            {
                m_queue->GetTimestampFrequency(&m_calibratedGpuTimestampFrequency);
            }
        }

        AZStd::pair<uint64_t, uint64_t> CommandQueue::GetClockCalibration()
        {
            AZStd::pair<uint64_t, uint64_t> calibratedTimestamp{ 0ull, 0ull };
            m_queue->GetClockCalibration(&calibratedTimestamp.first, &calibratedTimestamp.second);
            return calibratedTimestamp;
        }

        uint64_t CommandQueue::GetGpuTimestampFrequency() const
        {
            return m_calibratedGpuTimestampFrequency;
        }

        void CommandQueue::ExecuteWork(const RHI::ExecuteWorkRequest& rhiRequest)
        {
            auto& device = static_cast<Device&>(GetDevice());
            const ExecuteWorkRequest& request = static_cast<const ExecuteWorkRequest&>(rhiRequest);
            CommandQueueContext& context = device.GetCommandQueueContext();
            const FenceSet &compiledFences = context.GetCompiledFences();

            QueueCommand([=](void* commandQueue)
            {
                AZ_PROFILE_SCOPE(RHI, "ExecuteWork");
                AZ::Debug::ScopedTimer executionTimer(m_lastExecuteDuration);

                static const uint32_t CommandListCountMax = 128;
                ID3D12CommandQueue* dx12CommandQueue = static_cast<ID3D12CommandQueue*>(commandQueue);

                for (Fence* fence : request.m_userFencesToWaitFor)
                {
                    dx12CommandQueue->Wait(fence->Get(), fence->GetPendingValue());
                }

                for (size_t producerQueueIdx = 0; producerQueueIdx < request.m_waitFences.size(); ++producerQueueIdx)
                {
                    if (const uint64_t fenceValue = request.m_waitFences[producerQueueIdx])
                    {
                        const RHI::HardwareQueueClass producerQueueClass = static_cast<RHI::HardwareQueueClass>(producerQueueIdx);
                        m_queue->Wait(compiledFences.GetFence(producerQueueClass).Get(), fenceValue);
                    }
                }

                if (request.m_commandLists.size())
                {
                    uint32_t executeCount = 0;
                    ID3D12CommandList* executeLists[CommandListCountMax];

                    for (uint32_t commandListIdx = 0; commandListIdx < (uint32_t)request.m_commandLists.size(); ++commandListIdx)
                    {
                        CommandList& commandList = *request.m_commandLists[commandListIdx];

                        // Process tile mappings prior to executing the command lists.
                        if (commandList.HasTileMapRequests())
                        {
                            UpdateTileMappings(commandList);
                        }

                        executeLists[executeCount++] = commandList.GetCommandList();
                        commandList.SetParentQueue(this);
                    }
                    AZ_Assert(executeCount <= CommandListCountMax, "exceeded maximum number of command lists allowed");
                    dx12CommandQueue->ExecuteCommandLists(executeCount, executeLists);
                }

                if (request.m_signalFence > 0)
                {
                    dx12CommandQueue->Signal(compiledFences.GetFence(GetDescriptor().m_hardwareQueueClass).Get(), request.m_signalFence);
                }

                for (Fence* fence : request.m_userFencesToSignal)
                {
                    dx12CommandQueue->Signal(fence->Get(), fence->GetPendingValue());
                }

                AZ::Debug::ScopedTimer presentTimer(m_lastPresentDuration);
                for (RHI::DeviceSwapChain* swapChain : request.m_swapChainsToPresent)
                {
                    swapChain->Present();
                }
            });
        }

        void UpdateTileMap(RHI::Ptr<ID3D12CommandQueue> queue, const CommandList::TileMapRequest& request)
        {
            // map the source resource to null if the heap is null
            if (request.m_destinationHeap == nullptr)
            {
                // from: https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-updatetilemappings
                // If pRangeFlags[i] is D3D12_TILE_RANGE_FLAG_NULL,
                // pRangeTileCounts[i] specifies how many tiles from the tile regions to map to NULL.
                // If NumRanges is 1, pRangeTileCounts can be NULL and defaults to the total number of tiles
                // specified by all the tile regions. pHeapRangeStartOffsets[i] is ignored for NULL mappings.
                auto rangeFlag = D3D12_TILE_RANGE_FLAG_NULL;
                queue->UpdateTileMappings(
                    request.m_sourceMemory,
                    1, &request.m_sourceCoordinate, &request.m_sourceRegionSize,
                    nullptr,
                    1, &rangeFlag, nullptr, nullptr,
                    D3D12_TILE_MAPPING_FLAG_NONE);
            }
            else
            {
                // Maps a single range of source tiles to N disjoint destination tiles on a heap
                queue->UpdateTileMappings(
                    request.m_sourceMemory,
                    1, &request.m_sourceCoordinate, &request.m_sourceRegionSize,
                    request.m_destinationHeap,
                    aznumeric_cast<uint32_t>(request.m_rangeFlags.size()), request.m_rangeFlags.data(), request.m_rangeStartOffsets.data(), request.m_rangeTileCounts.data(),
                    D3D12_TILE_MAPPING_FLAG_NONE);
            }
        }

        void CommandQueue::UpdateTileMappings(CommandList& commandList)
        {
            AZ_PROFILE_SCOPE(RHI, "CommandQueue: UpdateTileMappings");
            for (const CommandList::TileMapRequest& request : commandList.GetTileMapRequests())
            {
                UpdateTileMap(m_queue, request);
            }
        }
        
        void CommandQueue::WaitForIdle()
        {
            AZ_PROFILE_SCOPE(RHI, "CommandQueue: WaitForIdle");

            Fence fence;
            fence.Init(m_device.get(), RHI::FenceState::Reset);
            QueueGpuSignal(fence);
            FlushCommands();

            FenceEvent event("WaitForIdle");
            fence.Wait(event);
        }

        void CommandQueue::ClearTimers()
        {
            m_lastExecuteDuration = 0;
        }

        AZStd::sys_time_t CommandQueue::GetLastExecuteDuration() const
        {
            return m_lastExecuteDuration;
        }

        AZStd::sys_time_t CommandQueue::GetLastPresentDuration() const
        {
            return m_lastPresentDuration;
        }
        
        void* CommandQueue::GetNativeQueue()
        {
            return m_queue.get();
        }
    }
}
