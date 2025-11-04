/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Instance.h>
#include <RHI/WSISurface.h>

#include <android/native_window.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode WSISurface::BuildNativeSurface()
        {
            Instance& instance = Instance::GetInstance();

            VkAndroidSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.window = reinterpret_cast<ANativeWindow*>(m_descriptor.m_windowHandle.GetIndex());
            const VkResult result = instance.GetContext().CreateAndroidSurfaceKHR(instance.GetNativeInstance(), &createInfo, VkSystemAllocator::Get(), &m_nativeSurface);
            AssertSuccess(result);

            return ConvertResult(result);
        }
    }
}
