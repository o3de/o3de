/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Instance.h>

namespace AZ::WebGPU
{
    wgpu::Instance Instance::BuildNativeInstance(const Instance::Descriptor& descriptor)
    {
        // Create the instance with the toggles
        AZStd::vector<const char*> enableToggleNames;
        AZStd::vector<const char*> disabledToggleNames;
        for (const AZStd::string& toggle : descriptor.m_enableToggles)
        {
            enableToggleNames.push_back(toggle.c_str());
        }

        for (const AZStd::string& toggle : descriptor.m_disableToggles)
        {
            disabledToggleNames.push_back(toggle.c_str());
        }
        wgpu::DawnTogglesDescriptor toggles = {};
        toggles.enabledToggles = enableToggleNames.data();
        toggles.enabledToggleCount = enableToggleNames.size();
        toggles.disabledToggles = disabledToggleNames.data();
        toggles.disabledToggleCount = disabledToggleNames.size();

        wgpu::InstanceDescriptor instanceDescriptor = {};
        instanceDescriptor.nextInChain = &toggles;
        instanceDescriptor.features.timedWaitAnyEnable = true;
        return wgpu::CreateInstance(&instanceDescriptor);
    }

}
