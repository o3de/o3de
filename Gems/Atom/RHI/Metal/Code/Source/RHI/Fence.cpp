/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Device.h>
#include <RHI/Fence.h>

namespace AZ
{
    namespace Metal
    {
        RHI::ResultCode Fence::Init(RHI::Ptr<Device> metalDevice, RHI::FenceState initialState)
        {
            m_device = metalDevice;
            id<MTLSharedEvent> mtlSharedEvent = [m_device->GetMtlDevice() newSharedEvent];
            if (mtlSharedEvent == nil)
            {
                AZ_Error("Fence", false, "Failed to initialize MTLSharedEvent.");
                return RHI::ResultCode::Fail;
            }

            m_fence = mtlSharedEvent;
            m_fence.signaledValue = 0u;
            m_pendingValue = (initialState == RHI::FenceState::Signaled) ? 0 : 1;
            return RHI::ResultCode::Success;
        }
        
        void Fence::Shutdown()
        {
            m_fence = nil;
            m_device = nullptr;
        }

        void Fence::WaitOnCpu() const
        {
            WaitOnCpu(m_pendingValue);
        }

        void Fence::WaitOnCpu(uint64_t fenceValue) const
        {
            if (GetCompletedValue() < fenceValue) // no need to wait if event has already reached the target value
            {
                MTLSharedEventListener* listener = m_device->GetSharedEventListener();
                dispatch_semaphore_t waitSemaphore = dispatch_semaphore_create(0);
                [m_fence notifyListener:listener atValue:fenceValue block:^(id<MTLSharedEvent> sharedEvent, uint64_t value)
                {
                    dispatch_semaphore_signal(waitSemaphore);
                }];
                dispatch_semaphore_wait(waitSemaphore, DISPATCH_TIME_FOREVER);
                dispatch_release(waitSemaphore);
            }
        }

        void Fence::WaitOnGpu(id<MTLCommandBuffer> waitingCommandBuffer) const
        {
            [waitingCommandBuffer encodeWaitForEvent:m_fence value:m_pendingValue];
        }

        void Fence::WaitOnGpu(id<MTLCommandBuffer> waitingCommandBuffer, uint64_t fenceValue) const
        {
            [waitingCommandBuffer encodeWaitForEvent:m_fence value:fenceValue];
        }

        void Fence::SignalFromCpu()
        { 
            SignalFromCpu(m_pendingValue);
        }

        void Fence::SignalFromCpu(uint64_t fenceValue)
        {
            m_fence.signaledValue = fenceValue;
        }

        void Fence::SignalFromGpu(id<MTLCommandBuffer> commandBufferToSignalFrom) const
        {
            SignalFromGpu(commandBufferToSignalFrom, m_pendingValue);
        }

        void Fence::SignalFromGpu(id<MTLCommandBuffer> commandBufferToSignalFrom, uint64_t fenceValue) const
        {
            [commandBufferToSignalFrom encodeSignalEvent:m_fence value:fenceValue];
        }

        RHI::FenceState Fence::GetFenceState() const
        {
            const uint64_t completedValue = GetCompletedValue();
            return (m_pendingValue <= completedValue) ? RHI::FenceState::Signaled : RHI::FenceState::Reset;
        }

        uint64_t Fence::GetPendingValue() const
        {
            return m_pendingValue;
        }

        uint64_t Fence::GetCompletedValue() const
        {
            return m_fence.signaledValue;
        }

        id<MTLSharedEvent> Fence::Get() const
        {
            return m_fence;
        }

        uint64_t Fence::Increment()
        {
            return ++m_pendingValue;
        }

        RHI::Ptr<FenceImpl> FenceImpl::Create()
        {
            return aznew FenceImpl();
        }

        RHI::ResultCode FenceImpl::InitInternal(RHI::Device& deviceBase, RHI::FenceState initialState, [[maybe_unused]] bool usedForWaitingOnDevice)
        {
            return m_fence.Init(&static_cast<Device&>(deviceBase), initialState);
        }

        void FenceImpl::ShutdownInternal()
        {
            m_fence.Shutdown();
        }

        void FenceImpl::ResetInternal()
        {
            m_fence.Increment();
        }

        void FenceImpl::SignalOnCpuInternal()
        {
            m_fence.SignalFromCpu();
        }

        void FenceImpl::WaitOnCpuInternal() const
        {
            m_fence.WaitOnCpu();
        }

        RHI::FenceState FenceImpl::GetFenceStateInternal() const
        {
            return m_fence.GetFenceState();
        }
    
        void FenceSet::Init(RHI::Ptr<Device> mtlDevice, RHI::FenceState initialState)
        {
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_fences[hardwareQueueIdx].Init(mtlDevice, initialState);
            }
        }

        void FenceSet::Shutdown()
        {
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_fences[hardwareQueueIdx].Shutdown();
            }
        }

        void FenceSet::Wait() const
        {
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_fences[hardwareQueueIdx].WaitOnCpu();
            }
        }

        void FenceSet::Reset()
        {
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_fences[hardwareQueueIdx].Increment();
            }
        }

        Fence& FenceSet::GetFence(RHI::HardwareQueueClass hardwareQueueClass)
        {
            return m_fences[static_cast<uint32_t>(hardwareQueueClass)];
        }

        const Fence& FenceSet::GetFence(RHI::HardwareQueueClass hardwareQueueClass) const
        {
            return m_fences[static_cast<uint32_t>(hardwareQueueClass)];
        }
    }
}
