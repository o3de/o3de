/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for input system notifications
    class InputSystemNotifications : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: input notifications are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: input notifications can be handled by multiple listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputSystemNotifications() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified right before the input system is about to tick input devices
        virtual void OnPreInputUpdate() {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified right after the input system is about to tick input devices
        virtual void OnPostInputUpdate() {}
    };
    using InputSystemNotificationBus = AZ::EBus<InputSystemNotifications>;
} // namespace AzFramework
