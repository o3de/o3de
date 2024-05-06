/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Semaphore.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        // Semaphore based on a timeline semaphore VkSemaphore
        // Is used if the device supports it and in all places except for the SwapChain
        // https://www.khronos.org/blog/vulkan-timeline-semaphores
        class TimelineSemaphore final : public Semaphore
        {
            using Base = Semaphore;

        public:
            AZ_RTTI(TimelineSemaphore, "{5480D230-A224-43E3-84BC-09E0B38AA638}", Semaphore);
            AZ_CLASS_ALLOCATOR(TimelineSemaphore, AZ::ThreadPoolAllocator);

            static RHI::Ptr<Semaphore> Create();
            ~TimelineSemaphore() = default;

            void WaitEvent() const override;

            uint64_t GetPendingValue();

        private:
            RHI::ResultCode InitInternal(Device& device) override;
            void ResetInternal() override;
            TimelineSemaphore() = default;

            uint64_t m_pendingValue = 0;
        };
    } // namespace Vulkan
} // namespace AZ
