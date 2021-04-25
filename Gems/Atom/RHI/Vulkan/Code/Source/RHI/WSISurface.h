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
#pragma once

#include <Atom/RHI/Object.h>
#include <Atom/RHI.Reflect/SwapChainDescriptor.h>

namespace AZ
{
    namespace Vulkan
    {
        class Instance;

        class WSISurface 
            : public RHI::Object
        {
            using Base = RHI::Object;

        public:
            AZ_CLASS_ALLOCATOR(WSISurface, AZ::SystemAllocator, 0);
            AZ_RTTI(WSISurface, "BFA18BB9-5BDA-46E7-AAAA-CEC9F965F1B8", Base);

            struct Descriptor
            {
                RHI::WindowHandle m_windowHandle;
            };

            static RHI::Ptr<WSISurface> Create();
            virtual RHI::ResultCode Init(const Descriptor& descriptor);
            ~WSISurface();

            VkSurfaceKHR GetNativeSurface() const;

        protected:
            WSISurface() = default;

            RHI::ResultCode BuildNativeSurface();

            Descriptor m_descriptor;
            VkSurfaceKHR m_nativeSurface = VK_NULL_HANDLE;
        };
    }
}
