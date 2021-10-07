/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Conversion.h>
#include <RHI/Instance.h>
#include <RHI/WSISurface.h>
#include <vulkan/vulkan.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode WSISurface::BuildNativeSurface()
        {
            Instance& instance = Instance::GetInstance();
            const HINSTANCE hinstance = GetModuleHandle(0);

            VkWin32SurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.hinstance = hinstance;
            createInfo.hwnd = reinterpret_cast<HWND>(m_descriptor.m_windowHandle.GetIndex());
            const VkResult result = vkCreateWin32SurfaceKHR(instance.GetNativeInstance(), &createInfo, nullptr, &m_nativeSurface);
            AssertSuccess(result);

            return ConvertResult(result);
        }
    }
}
