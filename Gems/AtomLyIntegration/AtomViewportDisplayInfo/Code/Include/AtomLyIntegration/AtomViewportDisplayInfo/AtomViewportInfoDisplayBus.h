/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
