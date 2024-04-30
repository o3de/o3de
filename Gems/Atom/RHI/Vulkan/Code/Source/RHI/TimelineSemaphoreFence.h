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

        // Fence based on a timeline semaphore VkSemaphore
        // Is used if the device supports it
        class TimelineSemaphoreFence final : public FenceBase
        {
            using Base = FenceBase;

        public:
            AZ_CLASS_ALLOCATOR(TimelineSemaphoreFence, AZ::ThreadPoolAllocator);
            AZ_RTTI(TimelineSemaphoreFence, "{B3FABCC5-24A7-43D0-868E-3C5E8FB6257A}", Base);

            static RHI::Ptr<FenceBase> Create();
            ~TimelineSemaphoreFence() = default;

            VkSemaphore GetNativeSemaphore() const;
            uint64_t GetPendingValue() const;

        private:
            TimelineSemaphoreFence() = default;

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

            VkSemaphore m_nativeSemaphore = VK_NULL_HANDLE;
            uint64_t m_pendingValue = 0;
        };
    } // namespace Vulkan
} // namespace AZ
