/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
