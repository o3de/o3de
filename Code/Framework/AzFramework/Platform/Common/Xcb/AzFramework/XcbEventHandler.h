/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/RTTI.h>

#include <xcb/xcb.h>

namespace AzFramework
{
    class XcbEventHandler
    {
    public:
        AZ_RTTI(XcbEventHandler, "{3F756E14-8D74-42FD-843C-4863307710DB}");

        virtual ~XcbEventHandler() = default;

        virtual void HandleXcbEvent(xcb_generic_event_t* event) = 0;

        // ATTN This is used as a workaround for RAW Input events when using the Editor.
        virtual void PollSpecialEvents(){};
    };

    class XcbEventHandlerBusTraits : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using XcbEventHandlerBus = AZ::EBus<XcbEventHandler, XcbEventHandlerBusTraits>;
} // namespace AzFramework
