/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>

#include <xcb/xcb.h>

namespace AzFramework
{
    class XcbConnectionManager
    {
    public:
        AZ_RTTI(XcbConnectionManager, "{1F756E14-8D74-42FD-843C-4863307710DB}");

        virtual ~XcbConnectionManager() = default;

        virtual xcb_connection_t* GetXcbConnection() const = 0;
    };

    class XcbConnectionManagerBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using XcbConnectionManagerBus = AZ::EBus<XcbConnectionManager, XcbConnectionManagerBusTraits>;
    using XcbConnectionManagerInterface = AZ::Interface<XcbConnectionManager>;
} // namespace AzFramework
