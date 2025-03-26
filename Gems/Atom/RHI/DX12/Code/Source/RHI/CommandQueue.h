/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/CommandQueue.h>
#include <Atom/RHI/Scope.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <RHI/CommandList.h>
#include <RHI/Fence.h>

namespace AZ
{
    namespace DX12
    {
        struct ExecuteWorkRequest
            : public RHI::ExecuteWorkRequest
        {
            static const uint64_t FenceValueNull = 0;

            /// Command lists to queue.
            AZStd::vector<CommandList*> m_commandLists;

            /// A set of fence values for each queue class to wait on
            /// before execution. Will ignore if null.
            FenceValueSet m_waitFences = { { FenceValueNull } };

            /// A fence value to signal after execution. Ignored if null.
            uint64_t m_signalFence = FenceValueNull;

            /// A set of user fences to signal after executing the command lists.
            AZStd::vector<Fence*> m_userFencesToSignal;

            /// A set of user fences to wait for before executing the command lists.
            AZStd::vector<Fence*> m_userFencesToWaitFor;
        };

        enum class HardwareQueueSubclass
        {
            Primary,
            Secondary,
        };

        struct CommandQueueDescriptor
            : public RHI::CommandQueueDescriptor
        {
            HardwareQueueSubclass m_hardwareQueueSubclass;
        };

        class CommandQueue final
            : public RHI::CommandQueue
        {
            using Base = RHI::CommandQueue;
        public:
            
            AZ_CLASS_ALLOCATOR(CommandQueue, AZ::SystemAllocator);
            AZ_RTTI(CommandQueue, "{9F93F430-E440-4033-9FBD-1A399B03355B}", Base);

            static RHI::Ptr<CommandQueue> Create();

            //////////////////////////////////////////////////////////////////////////
            // RHI::CommandQueue
            void ExecuteWork(const RHI::ExecuteWorkRequest& request) override;
            void WaitForIdle() override;
            //////////////////////////////////////////////////////////////////////////

            void CalibrateClock();
            AZStd::pair<uint64_t, uint64_t> GetClockCalibration();

            ID3D12CommandQueue* GetPlatformQueue() const;

            void QueueGpuSignal(Fence& fence);
            uint64_t GetGpuTimestampFrequency() const;

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

            
            void UpdateTileMappings(CommandList& commandList);

            RHI::Ptr<ID3D12CommandQueue> m_queue;
            RHI::Ptr<ID3D12DeviceX> m_device;

            HardwareQueueSubclass   m_hardwareQueueSubclass{ HardwareQueueSubclass::Primary };

            uint64_t m_calibratedGpuTimestampFrequency = 0;

            AZStd::sys_time_t m_lastExecuteDuration{};
            AZStd::sys_time_t m_lastPresentDuration{};
        };

        // helper function
        void UpdateTileMap(RHI::Ptr<ID3D12CommandQueue> queue, const CommandList::TileMapRequest& request);
    }
}
