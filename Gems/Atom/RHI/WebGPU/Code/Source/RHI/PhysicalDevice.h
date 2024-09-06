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
    namespace WebGPU
    {
        class PhysicalDevice final
            : public RHI::PhysicalDevice
        {
            using Base = RHI::PhysicalDevice;
        public:
            AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator);
            AZ_RTTI(PhysicalDevice, "{5AD2253C-07CD-4C37-BD21-482A49DB9E2B}", Base);
            PhysicalDevice() = default;
            ~PhysicalDevice() = default;

            static RHI::PhysicalDeviceList Enumerate();
            void Init(wgpu::Adapter& adapter);
            const wgpu::Adapter& GetNativeAdapter() const;

        private:
            wgpu::Adapter m_wgpuAdapter = nullptr;
            wgpu::AdapterInfo m_adapterInfo;
        };
    }
}
