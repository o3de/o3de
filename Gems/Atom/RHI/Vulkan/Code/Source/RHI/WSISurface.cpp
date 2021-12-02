/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WSISurface.h>
#include <RHI/Instance.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<WSISurface> WSISurface::Create()
        {
            return aznew WSISurface();
        }

        RHI::ResultCode WSISurface::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
            return BuildNativeSurface();
        }

        WSISurface::~WSISurface()
        {
            if (m_nativeSurface != VK_NULL_HANDLE)
            {
                Instance& instance = Instance::GetInstance();
                vkDestroySurfaceKHR(instance.GetNativeInstance(), m_nativeSurface, nullptr);
                m_nativeSurface = VK_NULL_HANDLE;
            }
        }

        VkSurfaceKHR WSISurface::GetNativeSurface() const
        {
            return m_nativeSurface;
        }
    }
}
