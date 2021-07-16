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

namespace AZ
{
    namespace AtomBridge
    {
        //! The level of information to display in the viewport info display overlay.
        enum class ViewportInfoDisplayState : int
        {
            NoInfo = 0,
            NormalInfo = 1,
            FullInfo = 2,
            CompactInfo = 3,
            Invalid
        };

        //! This bus is used to request changes to the viewport info display overlay.
        class AtomViewportInfoDisplayRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;

            //! Gets the current viewport info overlay state.
            virtual ViewportInfoDisplayState GetDisplayState() const = 0;
            //! Sets the current viewport info overlay state.
            //! The overlay will be drawn to the default viewport context every frame, if enabled.
            virtual void SetDisplayState(ViewportInfoDisplayState state) = 0;
        };

        using AtomViewportInfoDisplayRequestBus = AZ::EBus<AtomViewportInfoDisplayRequests>;

        //! This bus is used to listen for state changes in the viewport info display overlay.
        class AtomViewportInfoDisplayNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;

            //! Called when the ViewportInfoDisplayState (via the r_displayInfo CVar) has changed.
            virtual void OnViewportInfoDisplayStateChanged([[maybe_unused]]ViewportInfoDisplayState state){}
        };

        using AtomViewportInfoDisplayNotificationBus = AZ::EBus<AtomViewportInfoDisplayNotifications>;
    }
}
