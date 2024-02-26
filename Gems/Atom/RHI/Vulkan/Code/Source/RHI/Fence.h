/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Fence.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/SignalEvent.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        enum class FenceType
        {
            // Fence is based on VkFence
            // Cannot natively be signalled from the CPU
            // Signalling from the CPU is emulated by submitting a signal command to the Graphics queue
            // The signal command must also be submitted before we can wait for the fence to be signalled
            // Used if the device does not support timeline semaphores (Vulkan version < 1.2)
            Fence,
            // Fence is based on a timeline semaphore VkSemaphore
            // Is used if the device supports it
            TimelineSemaphore,
            Invalid
        };

        class Fence final : public RHI::Fence
        {
            using Base = RHI::Fence;

        public:
            AZ_CLASS_ALLOCATOR(Fence, AZ::ThreadPoolAllocator);
            AZ_RTTI(Fence, "AAAD0A37-5F85-4A68-9464-06EDAC6D62B0", Base);

            static RHI::Ptr<Fence> Create();
            ~Fence() = default;

            FenceType GetFenceType() const;

            // VkFence functions
            VkFence GetNativeFence() const;
            // According to the Vulkan Standard, fences can only be waited after the event
            // that will signal it is submitted. Because of this we need to tell the fence that it's
            // okay to start waiting on the native fence.
            void SignalEvent();

            // VkSemaphore functions
            VkSemaphore GetNativeSemaphore() const;
            uint64_t GetPendingValue() const;

        private:
            Fence() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////
            // RHI::Fence
            RHI::ResultCode InitInternal(RHI::Device& device, RHI::FenceState initialState) override;
            void ShutdownInternal() override;
            void SignalOnCpuInternal() override;
            void WaitOnCpuInternal() const override;
            void ResetInternal() override;
            RHI::FenceState GetFenceStateInternal() const override;
            //////////////////////////////////////////////////////////////////////

            FenceType m_fenceType = FenceType::Invalid;

            VkFence m_nativeFence = VK_NULL_HANDLE;
            AZ::Vulkan::SignalEvent m_signalEvent;

            VkSemaphore m_nativeSemaphore = VK_NULL_HANDLE;
            uint64_t m_pendingValue = 0;
        };
    } // namespace Vulkan
} // namespace AZ
