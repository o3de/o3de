/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PhysicalDevice.h>
#include <RHI/DX12.h>

namespace AZ
{
    namespace DX12
    {
        class PhysicalDevice final
            : public RHI::PhysicalDevice
        {
            using Base = RHI::PhysicalDevice;
        public:
            AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator);
            AZ_RTTI(PhysicalDevice, "{ACAE4F02-720E-4CAD-AECF-A999B3CAC49E}", Base);
            ~PhysicalDevice() = default;

            static RHI::PhysicalDeviceList Enumerate();

            IDXGIFactoryX* GetFactory() const;
            IDXGIAdapterX* GetAdapter() const;

        private:
            PhysicalDevice() = default;
            uint32_t GetGpuDriverVersion(const DXGI_ADAPTER_DESC& adapterDesc);

            void Init(IDXGIFactoryX* factory, IDXGIAdapterX* adapter);
            void Shutdown() override;

            RHI::Ptr<IDXGIFactoryX> m_dxgiFactory;
            RHI::Ptr<IDXGIAdapterX> m_dxgiAdapter;
        };
    }
}
