/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/API/ApplicationAPI_Platform.h>
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

            xcb_connection_t *  xcb_connection = NULL;
#if AZ_LINUX_WINDOW_MANAGER_XCB
            AzFramework::LinuxXCBConnectionManager::Bus::BroadcastResult(xcb_connection, &AzFramework::LinuxXCBConnectionManager::Bus::Events::GetXCBConnection);

            VkXcbSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.connection = xcb_connection;
            createInfo.window = static_cast<xcb_window_t>(m_descriptor.m_windowHandle.GetIndex());
            const VkResult result = vkCreateXcbSurfaceKHR(instance.GetNativeInstance(), &createInfo, nullptr, &m_nativeSurface);
            AssertSuccess(result);

            return ConvertResult(result);
#elif AZ_LINUX_WINDOW_MANAGER_WAYLAND
            #error "Linux Window Manager Wayland not supported."
            return RHI::ResultCode::Unimplemented;
#elif AZ_LINUX_WINDOW_MANAGER_XLIB
            #error "Linux Window Manager XLIB not supported."
            return RHI::ResultCode::Unimplemented;
#else
            #error "Linux Window Manager not recognized."
            return RHI::ResultCode::Unimplemented;
#endif // AZ_LINUX_WINDOW_MANAGER_XCB
        }
    }
}
