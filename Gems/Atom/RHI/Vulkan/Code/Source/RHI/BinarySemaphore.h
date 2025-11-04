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

        // Semaphore based on a basic VkSemaphore
        // Is used if the device does not support timeline semaphores and always for the SwapChain 
        // due to limitations discussed here: https://www.khronos.org/blog/vulkan-timeline-semaphores
        class BinarySemaphore final : public Semaphore
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_RTTI(BinarySemaphore, "{CA8937A8-98C8-4A6A-8C82-771145E4175C}", Semaphore);
            AZ_CLASS_ALLOCATOR(BinarySemaphore, AZ::ThreadPoolAllocator);

            static RHI::Ptr<Semaphore> Create();
            ~BinarySemaphore() = default;

        private:
            RHI::ResultCode InitInternal(Device& device) override;
            BinarySemaphore() = default;
        };
    } // namespace Vulkan
} // namespace AZ
