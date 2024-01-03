/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceFence.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/SignalEvent.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        class Fence final
            : public RHI::SingleDeviceFence
        {
            using Base = RHI::SingleDeviceFence;

        public:
            AZ_CLASS_ALLOCATOR(Fence, AZ::ThreadPoolAllocator);
            AZ_RTTI(Fence, "AAAD0A37-5F85-4A68-9464-06EDAC6D62B0", Base);

            static RHI::Ptr<Fence> Create();
            ~Fence() = default;

            // According to the Vulkan Standard, fences can only be waited after the event
            // that will signal it is submitted. Because of this we need to tell the fence that it's
            // okay to start waiting on the native fence.
            void SignalEvent();
            VkFence GetNativeFence() const;

        private:
            Fence() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////
            // RHI::SingleDeviceFence
            RHI::ResultCode InitInternal(RHI::Device& device, RHI::FenceState initialState) override;
            void ShutdownInternal() override;
            void SignalOnCpuInternal() override;
            void WaitOnCpuInternal() const override;
            void ResetInternal() override;
            RHI::FenceState GetFenceStateInternal() const override;
            //////////////////////////////////////////////////////////////////////

            VkFence m_nativeFence = VK_NULL_HANDLE;

            AZ::Vulkan::SignalEvent m_signalEvent;
        };
    }
}
