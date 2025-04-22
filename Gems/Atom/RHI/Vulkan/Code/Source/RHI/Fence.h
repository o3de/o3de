/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/FenceBase.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        // Fence is based on VkFence
        // Cannot natively be signalled from the CPU
        // Signalling from the CPU is emulated by submitting a signal command to the Graphics queue
        // The signal command must also be submitted before we can wait for the fence to be signalled
        // Used if the device does not support timeline semaphores (Vulkan version < 1.2)
        class Fence final : public RHI::DeviceFence
        {
            using Base = RHI::DeviceFence;

        public:
            AZ_CLASS_ALLOCATOR(Fence, AZ::ThreadPoolAllocator);
            AZ_RTTI(Fence, "{5B157619-B775-43D9-9112-B38F5B8011EC}", Base);

            static RHI::Ptr<Fence> Create();
            ~Fence() = default;

            void SetSignalEvent(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent);
            void SetSignalEventBitToSignal(int bitToSignal);
            void SetSignalEventDependencies(AZ::Vulkan::SignalEvent::BitSet dependencies);

            void SignalEvent();

            FenceBase& GetFenceBase() const;

        private:
            Fence() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////
            // RHI::DeviceFence
            RHI::ResultCode InitInternal(RHI::Device& device, RHI::FenceState initialState, bool usedForWaitingOnDevice) override;
            void ShutdownInternal() override;
            void SignalOnCpuInternal() override;
            void WaitOnCpuInternal() const override;
            void ResetInternal() override;
            RHI::FenceState GetFenceStateInternal() const override;
            void SetExternallySignalled() override;
            //////////////////////////////////////////////////////////////////////

            RHI::Ptr<FenceBase> m_fenceImpl;
        };
    } // namespace Vulkan
} // namespace AZ
