/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/Scope.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <Metal/Metal.h>

namespace AZ
{
    namespace Metal
    {
        class Device;

        //! A simple wrapper around MTLSharedEvent that also includes a monotonically increasing
        //! fence value.
        class Fence final
        {
        public:
            AZ_CLASS_ALLOCATOR(Fence, AZ::SystemAllocator);

            RHI::ResultCode Init(RHI::Ptr<Device> metalDevice, RHI::FenceState initialState);
            void Shutdown();

            uint64_t Increment();

            void SignalFromCpu();
            void SignalFromCpu(uint64_t fenceValueToSignal);
            void SignalFromGpu(id<MTLCommandBuffer> commandBufferToSignalFrom) const;
            void SignalFromGpu(id<MTLCommandBuffer> commandBufferToSignalFrom, uint64_t fenceValueToSignal) const;

            void WaitOnCpu() const;
            void WaitOnCpu(uint64_t fenceValue) const;
            void WaitOnGpu(id<MTLCommandBuffer> waitingCommandBuffer) const;
            void WaitOnGpu(id<MTLCommandBuffer> waitingCommandBuffer, uint64_t fenceValueToWaitFor) const;

            uint64_t GetPendingValue() const;

            uint64_t GetCompletedValue() const;
            RHI::FenceState GetFenceState() const;

            id<MTLSharedEvent> Get() const;
            
        private:
            RHI::Ptr<Device> m_device;
            id<MTLSharedEvent> m_fence = nil;
            uint64_t m_pendingValue = 1;
        };

        //! A simple utility wrapping a set of fences, one for each command queue.
        class FenceSet final
        {
        public:
            FenceSet() = default;

            void Init(RHI::Ptr<Device> metalDevice, RHI::FenceState initialState);

            void Shutdown();

            void Wait() const;

            void Reset();

            Fence& GetFence(RHI::HardwareQueueClass hardwareQueueClass);
            const Fence& GetFence(RHI::HardwareQueueClass hardwareQueueClass) const;

        private:
            AZStd::array<Fence, RHI::HardwareQueueClassCount> m_fences;
        };

        using FenceValueSet = AZStd::array<uint64_t, RHI::HardwareQueueClassCount>;

        //! The RHI fence implementation for Metal. 
        //! This exists separately from Fence to decouple
        //! the RHI::Device instance from low-level queue management. This is because RHI::DeviceFence holds
        //! a reference to the RHI device, which would create circular dependency issues if the device
        //! indirectly held a reference to one. Therefore, this implementation is only used when passing
        //! fences back and forth between the user and the RHI interface. Low-level systems will use
        //! the internal Fence instance instead.
        class FenceImpl final
            : public RHI::DeviceFence
        {
        public:
            AZ_CLASS_ALLOCATOR(FenceImpl, AZ::SystemAllocator);

            static RHI::Ptr<FenceImpl> Create();

            Metal::Fence& Get()
            {
                return m_fence;
            }

        private:
            FenceImpl() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceFence overrides ...
            RHI::ResultCode InitInternal(RHI::Device& device, RHI::FenceState initialState, bool usedForWaitingOnDevice) override;
            void ShutdownInternal() override;
            void SignalOnCpuInternal() override;
            void WaitOnCpuInternal() const override;
            void ResetInternal() override;
            RHI::FenceState GetFenceStateInternal() const override;
            //////////////////////////////////////////////////////////////////////////

            Metal::Fence m_fence;
        };
    }
}
