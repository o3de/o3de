/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// NOTE: We are careful to include platform headers *before* we include AzCore/Debug/Profiler.h to ensure that d3d12 symbols
// are defined prior to the inclusion of the pix3 runtime.
#include <RHI/DX12.h>

#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/Scope.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

namespace AZ
{
    namespace DX12
    {
        class Device;

        /**
         * A simple scoped wrapper around a HANDLE instance. Used as a context
         * for waiting on a fence.
         */
        class FenceEvent final
        {
        public:
            FenceEvent() = delete;
            FenceEvent(const char* name);
            ~FenceEvent();

            const char* GetName() const;

        private:
            friend class Fence;
            HANDLE m_EventHandle;
            const char* m_name;
        };

        /**
         * A simple wrapper around ID3D12Fence that also includes a monotonically increasing
         * fence value.
         */
        class Fence final
        {
        public:
            AZ_CLASS_ALLOCATOR(Fence, AZ::SystemAllocator);

            RHI::ResultCode Init(ID3D12DeviceX* dx12Device, RHI::FenceState initialState);

            void Shutdown();

            uint64_t Increment();

            void Signal();

            void Wait(FenceEvent& fenceEvent) const;
            void Wait(FenceEvent& fenceEvent, uint64_t fenceValue) const;

            uint64_t GetPendingValue() const;

            uint64_t GetCompletedValue() const;

            RHI::FenceState GetFenceState() const;

            ID3D12Fence* Get() const;

        private:
            RHI::Ptr<ID3D12Fence> m_fence;
            uint64_t m_pendingValue = 1;
        };

        /**
         * A simple utility wrapping a set of fences, one for each command queue.
         */
        class FenceSet final
        {
        public:
            FenceSet() = default;

            void Init(ID3D12DeviceX* dx12Device, RHI::FenceState initialState);

            void Shutdown();

            void Wait(FenceEvent& event) const;

            void Reset();

            Fence& GetFence(RHI::HardwareQueueClass hardwareQueueClass);
            const Fence& GetFence(RHI::HardwareQueueClass hardwareQueueClass) const;

        private:
            AZStd::array<Fence, RHI::HardwareQueueClassCount> m_fences;
        };

        using FenceValueSet = AZStd::array<uint64_t, RHI::HardwareQueueClassCount>;

        /**
         * The RHI fence implementation for DX12. This exists separately from Fence to decouple
         * the RHI::Device instance from low-level queue management. This is because RHI::DeviceFence holds
         * a reference to the RHI device, which would create circular dependency issues if the device
         * indirectly held a reference to one. Therefore, this implementation is only used when passing
         * fences back and forth between the user and the RHI interface. Low-level systems will use
         * the internal Fence instance instead.
         */
        class FenceImpl final
            : public RHI::DeviceFence
        {
        public:
            AZ_CLASS_ALLOCATOR(FenceImpl, AZ::SystemAllocator);
            AZ_RTTI(FenceImpl, "{6CD62A6F-FF00-4F6D-990B-59E220083939}", RHI::DeviceFence);

            static RHI::Ptr<FenceImpl> Create();

            DX12::Fence& Get()
            {
                return m_fence;
            }

        private:
            FenceImpl() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceFence
            RHI::ResultCode InitInternal(RHI::Device& device, RHI::FenceState initialState, bool usedForWaitingOnDevice) override;
            void ShutdownInternal() override;
            void SignalOnCpuInternal() override;
            void WaitOnCpuInternal() const override;
            void ResetInternal() override;
            RHI::FenceState GetFenceStateInternal() const override;
            //////////////////////////////////////////////////////////////////////////

            DX12::Fence m_fence;
        };
    }
}
