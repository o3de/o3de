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

        // BinaryFence is based on VkFence
        // Cannot natively be signalled from the CPU
        // Signalling from the CPU is emulated by submitting a signal command to the Graphics queue
        // The signal command must also be submitted before we can wait for the fence to be signalled
        // Used if the device does not support timeline semaphores (Vulkan version < 1.2)
        class BinaryFence final : public FenceBase
        {
            using Base = FenceBase;

        public:
            AZ_CLASS_ALLOCATOR(BinaryFence, AZ::ThreadPoolAllocator);
            AZ_RTTI(BinaryFence, "{FE8974F0-8C64-48A7-8BF2-89E92F911AA4}", Base);

            static RHI::Ptr<FenceBase> Create(Fence& fence);
            ~BinaryFence() = default;

            VkFence GetNativeFence() const;

        private:
            BinaryFence(Fence& fence)
                : m_fence{ fence } {};

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////
            // FenceBase
            RHI::ResultCode InitInternal(RHI::Device& device, RHI::FenceState initialState) override;
            void ShutdownInternal() override;
            void SignalOnCpuInternal() override;
            void WaitOnCpuInternal() const override;
            void ResetInternal() override;
            RHI::FenceState GetFenceStateInternal() const override;
            //////////////////////////////////////////////////////////////////////

            VkFence m_nativeFence = VK_NULL_HANDLE;
            Fence& m_fence;
        };
    } // namespace Vulkan
} // namespace AZ
