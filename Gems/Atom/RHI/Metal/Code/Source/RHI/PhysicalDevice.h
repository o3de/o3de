/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PhysicalDevice.h>
#include <Metal/Metal.h>

namespace AZ
{
    namespace Metal
    {
        class PhysicalDevice final
            : public RHI::PhysicalDevice
        {
            using Base = RHI::PhysicalDevice;
        public:
            AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator);
            AZ_RTTI(PhysicalDevice, "{1C0BFB27-F3A5-4B96-9497-29E80A954133}", Base);
            PhysicalDevice() = default;
            ~PhysicalDevice() = default;

            void Init(id<MTLDevice> mtlDevice);
            static RHI::PhysicalDeviceList Enumerate();
            id<MTLDevice> GetNativeDevice();

        private:
            void Shutdown() override;
            
            id<MTLDevice> m_mtlNativeDevice = nil;
        };
    }
}
