/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Vulkan_precompiled.h"
#include <RHI/Conversion.h>
#include <RHI/Instance.h>
#include <RHI/WSISurface.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode WSISurface::BuildNativeSurface()
        {
            Instance& instance = Instance::GetInstance();

            VkXcbSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.window = static_cast<xcb_window_t>(m_descriptor.m_windowHandle.GetIndex());
            const VkResult result = vkCreateXcbSurfaceKHR(instance.GetNativeInstance(), &createInfo, nullptr, &m_nativeSurface);
            AssertSuccess(result);

            return ConvertResult(result);
        }
    }
}
