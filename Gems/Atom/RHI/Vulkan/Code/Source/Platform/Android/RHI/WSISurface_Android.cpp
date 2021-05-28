/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "Atom_RHI_Vulkan_precompiled.h"
#include <RHI/Conversion.h>
#include <RHI/Instance.h>
#include <RHI/WSISurface.h>

#include <android/native_window.h>

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
            const VkResult result = vkCreateAndroidSurfaceKHR(instance.GetNativeInstance(), &createInfo, nullptr, &m_nativeSurface);
            AssertSuccess(result);

            return ConvertResult(result);
        }
    }
}
