/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Instance.h>
#include <RHI/WSISurface.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#include <AzFramework/XcbConnectionManager.h>
#endif

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode WSISurface::BuildNativeSurface()
        {
            Instance& instance = Instance::GetInstance();

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB

            xcb_connection_t* xcb_connection = nullptr;
            if (auto xcbConnectionManager = AzFramework::XcbConnectionManagerInterface::Get();
                xcbConnectionManager != nullptr)
            {
                xcb_connection = xcbConnectionManager->GetXcbConnection();
            }
            AZ_Error("AtomVulkan_RHI", xcb_connection!=nullptr, "Unable to get XCB Connection");

            VkXcbSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.connection = xcb_connection;
            createInfo.window = static_cast<xcb_window_t>(m_descriptor.m_windowHandle.GetIndex());
            const VkResult result = instance.GetContext().CreateXcbSurfaceKHR(instance.GetNativeInstance(), &createInfo, VkSystemAllocator::Get(), &m_nativeSurface);
            AssertSuccess(result);

            return ConvertResult(result);
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
            #error "Linux Window Manager Wayland not supported."
            return RHI::ResultCode::Unimplemented;
#else
            #error "Linux Window Manager not recognized."
            return RHI::ResultCode::Unimplemented;
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        }
    }
}
