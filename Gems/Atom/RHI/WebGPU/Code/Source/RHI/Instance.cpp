/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Instance.h>
#include <RHI/PhysicalDevice.h>

namespace AZ::WebGPU
{
    static EnvironmentVariable<Instance> s_webGPUInstance;
    static constexpr const char* s_webGPUInstanceKey = "WebGPUInstance";

    Instance& Instance::GetInstance()
    {
        if (!s_webGPUInstance)
        {
            s_webGPUInstance = Environment::FindVariable<Instance>(s_webGPUInstanceKey);
            if (!s_webGPUInstance)
            {
                s_webGPUInstance = Environment::CreateVariable<Instance>(s_webGPUInstanceKey);
            }
        }

        return s_webGPUInstance.Get();
    }

    void Instance::Reset()
    {
        s_webGPUInstance.Reset();
    }

    Instance::~Instance()
    {
        Shutdown();
    }
        
    bool Instance::Init(const Descriptor& descriptor)
    {
        m_descriptor = descriptor;
        wgpu::InstanceDescriptor instanceDescriptor = {};
        instanceDescriptor.features.timedWaitAnyEnable = true;

        m_wgpuInstance = BuildNativeInstance(descriptor);
        if (!m_wgpuInstance)
        {
            AZ_Warning("WebGPU", false, "Failed to build native instance.");
            return false;
        }

        m_supportedDevices = PhysicalDevice::Enumerate();
        AZ_Warning("WebGPU", !m_supportedDevices.empty(), "Could not find any WebGPU supported device");
        return !m_supportedDevices.empty();
    }

    void Instance::Shutdown() 
    {
        m_supportedDevices.clear();
    }

    const Instance::Descriptor& Instance::GetDescriptor() const
    {
        return m_descriptor;
    }

    wgpu::Instance& Instance::GetNativeInstance()
    {
        return m_wgpuInstance;
    }

    RHI::PhysicalDeviceList Instance::GetSupportedDevices() const
    {
        return m_supportedDevices;
    }
}
