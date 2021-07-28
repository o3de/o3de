/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/EBus/EBus.h>

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
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

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    class LinuxXcbConnectionManager
    {
    public:
        AZ_RTTI(LinuxXcbConnectionManager, "{649951316-3626-4C9D-9DCA-2E7ABF84C0A9}");

        virtual ~LinuxXcbConnectionManager() = default;

        virtual xcb_connection_t* GetXcbConnection() const = 0;
    };

    class LinuxXcbConnectionManagerBusTraits
        : public AZ::EBusTraits
    {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////
    };

    using LinuxXcbConnectionManagerBus = AZ::EBus<LinuxXcbConnectionManager, LinuxXcbConnectionManagerBusTraits>;
    using LinuxXcbConnectionManagerInterface = AZ::Interface<LinuxXcbConnectionManager>;

#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
} // namespace AzFramework
