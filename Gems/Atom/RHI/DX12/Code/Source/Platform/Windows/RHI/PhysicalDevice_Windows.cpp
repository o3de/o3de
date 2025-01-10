/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/PhysicalDevice_Windows.h>
#include <AzCore/std/string/conversions.h>

#include <Atom/RHI/RHIBus.h>

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

                DXGI_ADAPTER_DESC1 adapterDesc;
                dxgiAdapterX->GetDesc1(&adapterDesc);

                // Skip devices with software rasterization
                if(RHI::CheckBitsAny(adapterDesc.Flags, static_cast<UINT>(DXGI_ADAPTER_FLAG::DXGI_ADAPTER_FLAG_SOFTWARE)))
                {
                    continue;
                }

                PhysicalDevice* physicalDevice = aznew PhysicalDevice;
                physicalDevice->Init(dxgiFactory.Get(), dxgiAdapterX.Get());
                physicalDeviceList.emplace_back(physicalDevice);
            }

            RHI::RHIRequirementRequestBus::Broadcast(
                &RHI::RHIRequirementsRequest::FilterSupportedPhysicalDevices, physicalDeviceList, RHI::APIIndex::DX12);

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
            m_descriptor.m_vendorId = static_cast<RHI::VendorId>(adapterDesc.VendorId);
            m_descriptor.m_deviceId = adapterDesc.DeviceId;
            // Note: adapterDesc.Revision is not the driver version. It is "the PCI ID of the revision number of the adapter".
            m_descriptor.m_driverVersion = GetGpuDriverVersion(adapterDesc);
            m_descriptor.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Device)] = adapterDesc.DedicatedVideoMemory;
            m_descriptor.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Host)] = adapterDesc.DedicatedSystemMemory;
        }

        uint32_t PhysicalDevice::GetGpuDriverVersion(const DXGI_ADAPTER_DESC& adapterDesc)
        {
            HKEY dxKeyHandle = nullptr;

            LSTATUS returnCode = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectX"), 0, KEY_READ, &dxKeyHandle);

            if (returnCode != ERROR_SUCCESS)
            {
                return 0;
            }

            DWORD numOfAdapters = 0;

            returnCode = ::RegQueryInfoKey(
                dxKeyHandle, nullptr, nullptr, nullptr, &numOfAdapters, nullptr,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

            if (returnCode != ERROR_SUCCESS)
            {
                return 0;
            }

            constexpr uint32_t SubKeyLength = 256;
            wchar_t subKeyName[SubKeyLength];

            uint32_t driverVersion = 0;

            for (uint32_t i = 0; i < numOfAdapters; ++i)
            {
                DWORD subKeyLength = SubKeyLength;

                returnCode = ::RegEnumKeyEx(dxKeyHandle, i, subKeyName, &subKeyLength, nullptr, nullptr, nullptr, nullptr);

                if (returnCode == ERROR_SUCCESS)
                {
                    DWORD dwordSize = sizeof(uint32_t);
                    DWORD qwordSize = sizeof(uint64_t);
                    DWORD vendorIdRaw{};
                    DWORD deviceId{};
                    uint64_t driverVersionRaw{};

                    returnCode = ::RegGetValue(dxKeyHandle, subKeyName, TEXT("VendorId"), RRF_RT_DWORD, nullptr, &vendorIdRaw, &dwordSize);
                    if (returnCode != ERROR_SUCCESS)
                    {
                        continue;
                    }
                    returnCode = ::RegGetValue(dxKeyHandle, subKeyName, TEXT("DeviceId"), RRF_RT_DWORD, nullptr, &deviceId, &dwordSize);
                    if (returnCode != ERROR_SUCCESS)
                    {
                        continue;
                    }

                    if (vendorIdRaw != adapterDesc.VendorId || deviceId != adapterDesc.DeviceId)
                    {
                        continue;
                    }

                    returnCode = ::RegGetValue(dxKeyHandle, subKeyName, TEXT("DriverVersion"), RRF_RT_QWORD, nullptr, &driverVersionRaw, &qwordSize);

                    if (returnCode != ERROR_SUCCESS)
                    {
                        return 0;
                    }

                    RHI::VendorId vendorId = static_cast<RHI::VendorId>(vendorIdRaw);
                    // Full version number result in the following format:
                    // xx.xx.1x.xxxx (in decimal, each part takes 2 bytes in a QWORD/8 bytes)
                    // [operating system].[DX version].[Driver base line].[Build number]
                    // For the driver base line, different vendors have different format.
                    // For example, Nvidia uses 1x, but Intel uses 1xx.
                    // To align with Vulkan, we take the last 5 digits (in decimal) as the version number, as vendors usually do.
                    uint32_t baseline = (driverVersionRaw & 0xFFFF0000u) >> 16;
                    uint32_t buildNum = driverVersionRaw & 0x0000FFFFu;
                    switch (vendorId)
                    {
                    case RHI::VendorId::nVidia:
                        // From nVidia version format xx.xx.1x.xxxx
                        // to nVidia version format xxx.xx
                        // e.g 27.21.14.5687 -> 456.87 -> Vulkan format
                        driverVersion = (((baseline % 10) * 100 + buildNum / 100) << 22) | ((buildNum % 100) << 14);
                        break;
                    case RHI::VendorId::Intel:
                        // From nVidia version format xx.xx.1xx.xxxx
                        // to nVidia version format 1xx.xxxx
                        // e.g 25.20.100.6793 -> 100.6793 -> Vulkan format
                        driverVersion = (baseline << 14) | buildNum;
                        break;
                    default:
                        driverVersion = (baseline << 22) | (buildNum << 12);
                        
                    }
                    break;
                }
            }

            return driverVersion;
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
