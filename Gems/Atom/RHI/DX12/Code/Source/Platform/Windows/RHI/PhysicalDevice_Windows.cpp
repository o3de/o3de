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

#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/PhysicalDevice_Windows.h>
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    namespace DX12
    {
        RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
        {
            RHI::PhysicalDeviceList physicalDeviceList;

            DX12Ptr<IDXGIFactoryX> dxgiFactory;
            DX12::AssertSuccess(CreateDXGIFactory2(0, IID_GRAPHICS_PPV_ARGS(dxgiFactory.GetAddressOf())));

            DX12Ptr<IDXGIAdapter> dxgiAdapter;
            for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters(i, dxgiAdapter.ReleaseAndGetAddressOf()); ++i)
            {
                DX12Ptr<IDXGIAdapterX> dxgiAdapterX;
                dxgiAdapter->QueryInterface(IID_GRAPHICS_PPV_ARGS(dxgiAdapterX.GetAddressOf()));

                PhysicalDevice* physicalDevice = aznew PhysicalDevice;
                physicalDevice->Init(dxgiFactory.Get(), dxgiAdapterX.Get());
                physicalDeviceList.emplace_back(physicalDevice);
            }

            return physicalDeviceList;
        }

        void PhysicalDevice::Init(IDXGIFactoryX* factory, IDXGIAdapterX* adapter)
        {
            m_dxgiFactory = factory;
            m_dxgiAdapter = adapter;

            DXGI_ADAPTER_DESC adapterDesc;
            adapter->GetDesc(&adapterDesc);

            AZStd::string description;
            AZStd::to_string(description, adapterDesc.Description);

            m_descriptor.m_description = AZStd::move(description);
            m_descriptor.m_type = RHI::PhysicalDeviceType::Unknown; /// DXGI can't tell what kind of device this is?!
            m_descriptor.m_vendorId = adapterDesc.VendorId;
            m_descriptor.m_deviceId = adapterDesc.DeviceId;
            m_descriptor.m_driverVersion = adapterDesc.Revision;
            m_descriptor.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Device)] = adapterDesc.DedicatedVideoMemory;
            m_descriptor.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Host)] = adapterDesc.DedicatedSystemMemory;
        }

        void PhysicalDevice::Shutdown()
        {
            m_dxgiAdapter = nullptr;
            m_dxgiFactory = nullptr;
        }

        IDXGIFactoryX* PhysicalDevice::GetFactory() const
        {
            return m_dxgiFactory.get();
        }

        IDXGIAdapterX* PhysicalDevice::GetAdapter() const
        {
            return m_dxgiAdapter.get();
        }
    }
}
