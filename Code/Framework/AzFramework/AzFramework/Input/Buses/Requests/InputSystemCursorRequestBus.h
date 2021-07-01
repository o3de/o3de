/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! State of the system cursor
    enum class SystemCursorState
    {
        Unknown,                //!< The state of the system cursor is not known
        ConstrainedAndHidden,   //!< Constrained to the application's main window and hidden
        ConstrainedAndVisible,  //!< Constrained to the application's main window and visible
        UnconstrainedAndHidden, //!< Free to move outside the main window but hidden while inside
        UnconstrainedAndVisible //!< Free to move outside the application's main window and visible
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to query/change the state, position, or appearance of the system cursor
    class InputSystemCursorRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId using EBus<>::Event,
        //! which should be handled by only one device that has connected to the bus using that id.
        //! Input requests can also be sent using EBus<>::Broadcast, in which case they'll be sent
        //! to all input devices that have connected to the input event bus regardless of their id.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests should be handled by only one input device connected to each id
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId
        using BusIdType = InputDeviceId;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Inform input devices that the system cursor state should be changed. Calls to the method
        //! should usually be addressed to a mouse input device, but it may be possible for multiple
        //! different device instances to each be associated with a system cursor.
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \param[in] systemCursorState The desired system cursor state
        virtual void SetSystemCursorState(SystemCursorState /*systemCursorState*/) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query input devices for the current state of the system cursor. All calls to this method
        //! should usually be addressed to a mouse input device, but it may be possible for multiple
        //! different device instances to each be associated with a system cursor.
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \return The current state of the system cursor
        virtual SystemCursorState GetSystemCursorState() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Inform input devices that the system cursor position should be set. Calls to the method
        //! should usually be addressed to a mouse input device, but it may be possible for multiple
        //! different device instances to each be associated with a system cursor.
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \param[in] positionNormalized The desired system cursor position normalized
        virtual void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the system cursor position normalized relative to the application's main window. The
        //! position obtained has had os ballistics applied, and is valid regardless of whether the
        //! system cursor is hidden or visible. When the cursor is constrained to the application's
        //! main window the values will always be in the [0.0, 1.0] range, but if unconstrained the
        //! normalized values will not be clamped so they will exceed this range anytime the system
        //! cursor is located outside the application's main window.
        //! See also InputSystemCursorRequests::SetSystemCursorState and GetSystemCursorState.
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \return The system cursor position normalized relative to the application's main window
        virtual AZ::Vector2 GetSystemCursorPositionNormalized() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputSystemCursorRequests() = default;
    };
    using InputSystemCursorRequestBus = AZ::EBus<InputSystemCursorRequests>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface to request the window or view used to clip and/or normalize the system cursor
    class InputSystemCursorConstraintRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests are handled by a single listener
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputSystemCursorConstraintRequests() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the application window/view that should be used to clip and/or normalize the cursor.
        //!
        //! Ideally there should be an abstract cross-platform 'AzFramework::ApplicationWindow/View'
        //! class/interface that systems like input and rendering use to interact with the windowing
        //! system in a platform agnostic manner, but for now we will make do with returning a void*
        //! and depend on the caller to cast it to the appropriate platform specific representation.
        //!
        //! \return Pointer to a platform specific representation of the application window or view
        //! that should be used to clip and/or normalize the system cursor. If nullptr is returned,
        //! the default main window/view will be used instead. Return type depends on the platform,
        //! as does the fallback main window/view that will be used instead if nullptr is returned:
        //! - Windows: HWND (fallback ::GetFocus())
        //! - macOS: NSView (fallback NSApplication.sharedApplication.mainWindow.contentView)
        virtual void* GetSystemCursorConstraintWindow() const = 0;
    };
    using InputSystemCursorConstraintRequestBus = AZ::EBus<InputSystemCursorConstraintRequests>;
} // namespace AzFramework
