/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/Instance.h>
#include <RHI/Conversions.h>

namespace AZ
{
    namespace WebGPU
    {
        RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
        {
            // Setup base adapter options.
            wgpu::RequestAdapterOptions adapterOptions = {};
            adapterOptions.backendType = wgpu::BackendType::Undefined;
            adapterOptions.powerPreference = wgpu::PowerPreference::HighPerformance;

            // Synchronously create the adapter
            auto& instance = Instance::GetInstance().GetNativeInstance();
            wgpu::Adapter wgpuAdapter = nullptr;
            instance.WaitAny(
                instance.RequestAdapter(
                    &adapterOptions,
                    wgpu::CallbackMode::WaitAnyOnly,
                    [&wgpuAdapter](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, const char* message)
                    {
                        if (status != wgpu::RequestAdapterStatus::Success)
                        {
                            AZ_Warning("WebGPU", false, "Failed to get an adapter: %s", message);
                            return;
                        }
                        wgpuAdapter = AZStd::move(adapter);
                    }),
                UINT64_MAX);

            RHI::PhysicalDeviceList physicalDeviceList;
            if (wgpuAdapter)
            {
                RHI::Ptr<PhysicalDevice> physicalDevice = aznew PhysicalDevice;
                physicalDevice->Init(wgpuAdapter);
                physicalDeviceList.emplace_back(physicalDevice);
            }

            return physicalDeviceList;
        }

        void PhysicalDevice::Init(wgpu::Adapter& adapter)
        {
            AZ_Assert(adapter, "Invalid adapter when initializing PhysicalDevice");
            m_wgpuAdapter = AZStd::move(adapter);

            m_wgpuAdapter.GetInfo(&m_adapterInfo);
            AZ_Printf("WebGPU", "Using adapter %s with backend %s", m_adapterInfo.device, ToString(m_adapterInfo.backendType));

            m_wgpuAdapter.GetLimits(&m_limits);
        }

        const wgpu::Adapter& PhysicalDevice::GetNativeAdapter() const
        {
            return m_wgpuAdapter;
        }

        const wgpu::SupportedLimits& PhysicalDevice::GetLimits() const
        {
            return m_limits;
        }
    }
}
