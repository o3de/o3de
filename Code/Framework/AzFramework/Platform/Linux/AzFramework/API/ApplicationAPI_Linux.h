/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/EBus.h>

#if AZ_LINUX_WINDOW_MANAGER_XCB
#include <xcb/xcb.h>
#endif // LY_COMPILE_DEFINITIONS

namespace AzFramework
{
    class LinuxLifecycleEvents
        : public AZ::EBusTraits
    {
    public:
        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~LinuxLifecycleEvents() {}

        using Bus = AZ::EBus<LinuxLifecycleEvents>;
    };

#if AZ_LINUX_WINDOW_MANAGER_XCB
    class LinuxXCBConnectionManager
       : public AZ::EBusTraits
    {
    public:
        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~LinuxXCBConnectionManager() {}

        using Bus = AZ::EBus<LinuxXCBConnectionManager>;

        virtual xcb_connection_t* GetXCBConnection() const = 0;

    };
#endif // AZ_LINUX_WINDOW_MANAGER_XCB
} // namespace AzFramework
