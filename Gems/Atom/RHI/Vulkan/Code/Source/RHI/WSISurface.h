/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom_RHI_Vulkan_Platform.h>
#include <Atom/RHI/Object.h>
#include <Atom/RHI.Reflect/SwapChainDescriptor.h>
#include <RHI/WindowSurfaceBus.h>

namespace AZ
{
    namespace Vulkan
    {
        class Instance;

        class WSISurface 
            : public RHI::Object
            , public WindowSurfaceRequestsBus::Handler
        {
            using Base = RHI::Object;

        public:
            AZ_CLASS_ALLOCATOR(WSISurface, AZ::SystemAllocator);
            AZ_RTTI(WSISurface, "BFA18BB9-5BDA-46E7-AAAA-CEC9F965F1B8", Base);

            struct Descriptor
            {
                RHI::WindowHandle m_windowHandle;
            };

            static RHI::Ptr<WSISurface> Create();
            virtual RHI::ResultCode Init(const Descriptor& descriptor);
            ~WSISurface();

            // WindowSurfaceRequestsBus::Handler overrides...
            VkSurfaceKHR GetNativeSurface() const override;

        protected:
            WSISurface() = default;

            RHI::ResultCode BuildNativeSurface();

            Descriptor m_descriptor;
            VkSurfaceKHR m_nativeSurface = VK_NULL_HANDLE;
        };
    }
}
