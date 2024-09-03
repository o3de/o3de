/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Instance.h>
#include <RHI/Swapchain.h>
#include <AzCore/PlatformIncl.h>

namespace AZ::WebGPU
{
    wgpu::Surface SwapChain::BuildNativeSurface(const RHI::SwapChainDescriptor& descriptor)
    {
        wgpu::SurfaceDescriptorFromWindowsHWND desc = {};
        desc.hwnd = reinterpret_cast<HWND>(descriptor.m_window.GetIndex());
        desc.hinstance = GetModuleHandle(0);

        wgpu::SurfaceDescriptor surfaceSescriptor;
        surfaceSescriptor.nextInChain = &desc;
        wgpu::Surface surface = Instance::GetInstance().GetNativeInstance().CreateSurface(&surfaceSescriptor);

        return surface;
    }
}
