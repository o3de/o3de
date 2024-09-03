/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <Atom/RHI/PhysicalDevice.h>

namespace AZ::WebGPU
{
    class Instance final
    {
    public:
        AZ_CLASS_ALLOCATOR(Instance, AZ::SystemAllocator);

        struct Descriptor
        {
            AZStd::vector<AZStd::string> m_enableToggles;
            AZStd::vector<AZStd::string> m_disableToggles;
        };

        static Instance& GetInstance();
        static void Reset();
        ~Instance();
        bool Init(const Descriptor& descriptor);
        void Shutdown();

        const Descriptor& GetDescriptor() const;
        wgpu::Instance& GetNativeInstance();
        RHI::PhysicalDeviceList GetSupportedDevices() const;

    private:
        static wgpu::Instance BuildNativeInstance(const Descriptor& descriptor);

        Descriptor m_descriptor;
        wgpu::Instance m_wgpuInstance = nullptr;
        RHI::PhysicalDeviceList m_supportedDevices;
    };
} // namespace AZ::WebGPU
