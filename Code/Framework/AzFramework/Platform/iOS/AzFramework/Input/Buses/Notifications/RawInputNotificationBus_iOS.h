/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

@class UITouch;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for raw ios input events broadcast by the system. Applications
    //! that want raw ios events to be processed by the AzFramework input system must broadcast all
    //! input events received by a UIResponder instance, which is the lowest level we can get input.
    //!
    //! Input events should only be broadcast by a single UIResponder object, ideally from a custom
    //! UIApplication object (which is guaranteed to be the last object in the ios responder chain).
    //! This ensures that any Cocoa/UIKit controls that might be active are allowed the opportunity
    //! to process input events before the engine (even though this scenario is unlikely for a game).
    //!
    //! It's possible to receive multiple touch events per index (finger) per frame, and while it is
    //! possible to pump the ios event loop from any thread it is only possible to process raw touch
    //! events received by a UIResponder instance, which is guaranteed to happen on the main thread.
    //!
    //! This EBus is intended primarily for the AzFramework input system to process ios input events.
    //! Most systems that need to process input should use the generic AzFramework input interfaces,
    //! but if necessary it is perfectly valid to connect directly to this EBus for raw ios events.
    class RawInputNotificationsIos : public AZ::EBusTraits
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
        virtual ~RawInputNotificationsIos() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Process raw touch events (that will always be dispatched on the main thread)
        //! \param[in] uiTouch The raw touch data
        virtual void OnRawTouchEventBegan(const UITouch* /*uiTouch*/) {}
        virtual void OnRawTouchEventMoved(const UITouch* /*uiTouch*/) {}
        virtual void OnRawTouchEventEnded(const UITouch* /*uiTouch*/) {}
        ///@}
    };
    using RawInputNotificationBusIos = AZ::EBus<RawInputNotificationsIos>;
} // namespace AzFramework
