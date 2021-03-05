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