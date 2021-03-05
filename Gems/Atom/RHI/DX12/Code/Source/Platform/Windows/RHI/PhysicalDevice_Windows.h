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

namespace AZ
{
    namespace DX12
    {
        class PhysicalDevice final
            : public RHI::PhysicalDevice
        {
            using Base = RHI::PhysicalDevice;
        public:
            AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator, 0);
            AZ_RTTI(PhysicalDevice, "{ACAE4F02-720E-4CAD-AECF-A999B3CAC49E}", Base);
            ~PhysicalDevice() = default;

            static RHI::PhysicalDeviceList Enumerate();

            IDXGIFactoryX* GetFactory() const;
            IDXGIAdapterX* GetAdapter() const;

        private:
            PhysicalDevice() = default;

            void Init(IDXGIFactoryX* factory, IDXGIAdapterX* adapter);
            void Shutdown() override;

            RHI::Ptr<IDXGIFactoryX> m_dxgiFactory;
            RHI::Ptr<IDXGIAdapterX> m_dxgiAdapter;
        };
    }
}