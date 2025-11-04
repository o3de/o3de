/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WSISurface.h>
#include <RHI/Instance.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

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
            RHI::ResultCode result = BuildNativeSurface();
            if (result == RHI::ResultCode::Success)
            {
                WindowSurfaceRequestsBus::Handler::BusConnect(descriptor.m_windowHandle);
            }
            return result;
        }

        WSISurface::~WSISurface()
        {
            if (m_nativeSurface != VK_NULL_HANDLE)
            {
                WindowSurfaceRequestsBus::Handler::BusDisconnect(m_descriptor.m_windowHandle);
                Instance& instance = Instance::GetInstance();
                instance.GetContext().DestroySurfaceKHR(instance.GetNativeInstance(), m_nativeSurface, VkSystemAllocator::Get());
                m_nativeSurface = VK_NULL_HANDLE;
            }
        }

        VkSurfaceKHR WSISurface::GetNativeSurface() const
        {
            return m_nativeSurface;
        }
    }
}
