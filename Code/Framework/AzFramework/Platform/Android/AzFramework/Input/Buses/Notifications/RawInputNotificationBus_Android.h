/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

struct AInputEvent;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for raw android input as broadcast by the system. Applications
    //! that want android events to be processed by the AzFramework input system must broadcast all
    //! input events received by the native input handle, which is the lowest level we can get input.
    //!
    //! It's possible to receive multiple events per index (finger) per frame, and it is likely that
    //! android input events will not be dispatched from the main thread, so care should be taken to
    //! ensure thread safety when implementing event handlers that connect to this android event bus.
    //!
    //! This EBus is intended primarily for the AzFramework input system to process raw input events.
    //! Most systems that need to process input should use the generic AzFramework input interfaces,
    //! but if necessary it is perfectly valid to connect directly to this EBus for android events.
    class RawInputNotificationsAndroid : public AZ::EBusTraits
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
        virtual ~RawInputNotificationsAndroid() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw input events (assumed to be dispatched on any thread)
        //! \param[in] rawInputEvent The raw input event data
        virtual void OnRawInputEvent(const AInputEvent* /*rawInputEvent*/) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw text events (assumed to be dispatched on any thread)
        //! \param[in] charsModifiedUTF8 The raw chars (encoded using modified UTF-8)
        virtual void OnRawInputTextEvent(const char* /*charsModifiedUTF8*/) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw virtual keyboard events (assumed to be dispatched on any thread)
        //! \param[in] keyCode The key code of the virtual keyboard event
        //! \param[in] keyAction The key action of the virtual keyboard event
        virtual void OnRawInputVirtualKeyboardEvent(int /*keyCode*/, int /*keyAction*/) {}
    };
    using RawInputNotificationBusAndroid = AZ::EBus<RawInputNotificationsAndroid>;
} // namespace AzFramework
