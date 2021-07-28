/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace BarrierInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Barrier keyboard modifier bit mask
    enum ModifierMask
    {
        ModifierMask_None       = 0x0000,
        ModifierMask_Shift      = 0x0001,
        ModifierMask_Ctrl       = 0x0002,
        ModifierMask_AltL       = 0x0004,
        ModifierMask_Windows    = 0x0010,
        ModifierMask_AltR       = 0x0020,
        ModifierMask_CapsLock   = 0x1000,
        ModifierMask_NumLock    = 0x2000,
        ModifierMask_ScrollLock = 0x4000,
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for raw Barrier input as broadcast by the BarrierClient.
    //!
    //! It's possible to receive multiple events per button/key per frame, and it's very likely that
    //! Barrier input events will not be dispatched from the main thread, so care should be taken to
    //! ensure thread safety when implementing event handlers that connect to this Barrier event bus.
    //!
    //! This EBus is intended primarily for the BarrierClient to send raw input to Barrier devices.
    //! Most systems that need to process input should use the generic AzFramework input interfaces,
    //! but if necessary it is perfectly valid to connect directly to this EBus for Barrier events.
    class RawInputNotificationsBarrier : public AZ::EBusTraits
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
        virtual ~RawInputNotificationsBarrier() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw mouse button down events (assumed to be dispatched from any thread)
        //! \param[in] buttonIndex The index of the button that was pressed down
        virtual void OnRawMouseButtonDownEvent([[maybe_unused]]uint32_t buttonIndex) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw mouse button up events (assumed to be dispatched from any thread)
        //! \param[in] buttonIndex The index of the button that was released up
        virtual void OnRawMouseButtonUpEvent([[maybe_unused]]uint32_t buttonIndex) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw mouse movement events (assumed to be dispatched from any thread)
        //! \param[in] movementX The x movement of the mouse
        //! \param[in] movementY The y movement of the mouse
        virtual void OnRawMouseMovementEvent([[maybe_unused]]float movementX, [[maybe_unused]]float movementY) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw mouse position events (assumed to be dispatched from any thread)
        //! \param[in] positionX The x position of the mouse
        //! \param[in] positionY The y position of the mouse
        virtual void OnRawMousePositionEvent([[maybe_unused]]float positionX, [[maybe_unused]]float positionY) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw keyboard key down events (assumed to be dispatched from any thread)
        //! \param[in] scanCode The scan code of the key that was pressed down
        //! \param[in] activeModifiers The bit mask of currently active modifier keys
        virtual void OnRawKeyboardKeyDownEvent([[maybe_unused]]uint32_t scanCode, [[maybe_unused]]ModifierMask activeModifiers) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw keyboard key up events (assumed to be dispatched from any thread)
        //! \param[in] scanCode The scan code of the key that was released up
        //! \param[in] activeModifiers The bit mask of currently active modifier keys
        virtual void OnRawKeyboardKeyUpEvent([[maybe_unused]]uint32_t scanCode, [[maybe_unused]]ModifierMask activeModifiers) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw keyboard key repeat events (assumed to be dispatched from any thread)
        //! \param[in] scanCode The scan code of the key that was repeatedly held down
        //! \param[in] activeModifiers The bit mask of currently active modifier keys
        virtual void OnRawKeyboardKeyRepeatEvent([[maybe_unused]]uint32_t scanCode, [[maybe_unused]]ModifierMask activeModifiers) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw clipboard events (assumed to be dispatched from any thread)
        //! \param[in] clipboardContents The contents of the clipboard
        virtual void OnRawClipboardEvent([[maybe_unused]]const char* clipboardContents) {}
    };
    using RawInputNotificationBusBarrier = AZ::EBus<RawInputNotificationsBarrier>;
} // namespace BarrierInput
