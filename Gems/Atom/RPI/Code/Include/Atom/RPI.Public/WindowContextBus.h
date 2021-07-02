/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace RPI
    {
        class WindowContextNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AzFramework::NativeWindowHandle;

            virtual void OnViewportResized(uint32_t width, uint32_t height) = 0;
        };

        using WindowContextNotificationBus = AZ::EBus<WindowContextNotifications>;
    } // namespace RPI
} // namespace AZ
