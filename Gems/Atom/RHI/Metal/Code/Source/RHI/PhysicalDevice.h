/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator, 0);
            AZ_RTTI(PhysicalDevice, "{1C0BFB27-F3A5-4B96-9497-29E80A954133}", Base);
            PhysicalDevice() = default;
            ~PhysicalDevice() = default;

            void Init(id<MTLDevice> mtlDevice);
            static RHI::PhysicalDeviceList Enumerate();
        private:
            void Shutdown() override;
        };
    }
}
