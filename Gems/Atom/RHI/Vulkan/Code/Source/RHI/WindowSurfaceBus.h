/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <Atom/RHI.Reflect/SwapChainDescriptor.h>

namespace AZ
{
    namespace Vulkan
    {
        //! For requesting the WSISurface of a Native Window
        class WindowSurfaceRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            using BusIdType = RHI::WindowHandle;

            //! Returns the Vulkan Surface that the window is connected to.
            virtual VkSurfaceKHR GetNativeSurface() const = 0;
        };

        using WindowSurfaceRequestsBus = AZ::EBus<WindowSurfaceRequests>;
    } // namespace Vulkan
} // namespace AZ
