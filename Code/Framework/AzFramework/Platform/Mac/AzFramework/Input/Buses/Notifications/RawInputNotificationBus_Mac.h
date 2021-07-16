/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

@class NSEvent;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for raw Mac input events broadcast by the system. Applications
    //! that want raw Mac events to be processed by the AzFramework input system must broadcast all
    //! events received when pumping the NSEvent loop, which is the lowest level we can access input.
    //!
    //! It's possible to receive multiple events per button/key per frame and (depending on how the
    //! NSEvent event loop is pumped) it is also possible that events could be sent from any thread,
    //! however it is assumed they'll always be dispatched on the main thread which is the standard.
    //!
    //! This EBus is intended primarily for the AzFramework input system to process Mac input events.
    //! Most systems that need to process input should use the generic AzFramework input interfaces,
    //! but if necessary it is perfectly valid to connect directly to this EBus for raw Mac events.
    class RawInputNotificationsMac : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: raw input notifications are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: raw input notifications can be handled by multiple listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~RawInputNotificationsMac() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw input events (assumed to be dispatched on the main thread)
        //! \param[in] nsEvent The raw event data
        virtual void OnRawInputEvent(const NSEvent* /*nsEvent*/) = 0;
    };
    using RawInputNotificationBusMac = AZ::EBus<RawInputNotificationsMac>;
} // namespace AzFramework
