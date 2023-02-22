/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PhysicalDevice.h>

namespace AZ
{
    namespace Null
    {
        class PhysicalDevice final
            : public RHI::PhysicalDevice
        {
            using Base = RHI::PhysicalDevice;
        public:
            AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator);
            AZ_RTTI(PhysicalDevice, "{BE6A13AB-4D5D-4B2C-8869-7EEF11462287}", Base);
            PhysicalDevice() = default;
            ~PhysicalDevice() = default;

            static RHI::PhysicalDeviceList Enumerate();
        };
    }
}
