/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/parallel/thread.h>

#include <AppKit/NSApplication.h>
#include <AppKit/NSEvent.h>
#include <AppKit/NSScreen.h>
#include <AppKit/NSView.h>
#include <AppKit/NSWindow.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // NSEventType enum constant names were changed in macOS 10.12, but our min-spec is still 10.10
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 101200 // __MAC_10_12 may not be defined by all earlier sdks
    static const NSEventType NSEventTypeLeftMouseDown       = NSLeftMouseDown;
    static const NSEventType NSEventTypeLeftMouseUp         = NSLeftMouseUp;
    static const NSEventType NSEventTypeRightMouseDown      = NSRightMouseDown;
    static const NSEventType NSEventTypeRightMouseUp        = NSRightMouseUp;
    static const NSEventType NSEventTypeOtherMouseDown      = NSOtherMouseDown;
    static const NSEventType NSEventTypeOtherMouseUp        = NSOtherMouseUp;
    static const NSEventType NSEventTypeScrollWheel         = NSScrollWheel;
    static const NSEventType NSEventTypeMouseMoved          = NSMouseMoved;
    static const NSEventType NSEventTypeLeftMouseDragged    = NSLeftMouseDragged;
    static const NSEventType NSEventTypeRightMouseDragged   = NSRightMouseDragged;
    static const NSEventType NSEventTypeOtherMouseDragged   = NSOtherMouseDragged;
#endif // __MAC_OS_X_VERSION_MAX_ALLOWED < 101200

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get the main application view that should be used to clip and/or normalize the cursor.
    //! \return The NSView that should currently be considered as the applictaion's main view.
    NSView* GetSystemCursorMainContentView()
    {
        void* systemCursorMainContentView = nullptr;
        AzFramework::InputSystemCursorConstraintRequestBus::BroadcastResult(
            systemCursorMainContentView,
            &AzFramework::InputSystemCursorConstraintRequests::GetSystemCursorConstraintWindow);
        return systemCursorMainContentView ?
               static_cast<NSView*>(systemCursorMainContentView) :
               NSApplication.sharedApplication.mainWindow.contentView;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Convert point from the global screen co-ordinate space of the Core Graphics Framework to the
    //! local co-ordinate space of an NSView. It is assumed the y co-ordinate of the point passed to
    //! this function is measured from the top-left corner of the screen, and all necessary flipping
    //! operations will be performed internally to ensure the y co-ordinate of the point returned is
    //! measured from the top-left corner of the NSView (which may not be expected if then used with
    //! an NSView that is not using flipped co-ordinates, so care should be taken using the result).
    //! However, the optional 'relativeToScreenTop' arg can instead be set to false to indicate that
    //! pointInScreenSpace should be assumed relative to the bottom-left of the screen.
    //! \param[in] pointInScreenSpace A point relative to the top-left corner of global screen space
    //! \param[in] view The view whose local co-ordinate space the screen point will be converted to
    //! \param[in] relativeToScreenTop True if the point is relative to the screen top, false bottom
    //! \return The point relative to the top-left corner of the local co-ordinate space of the view
    NSPoint ConvertPointFromScreenSpaceToViewSpace(const NSPoint& pointInScreenSpace,
                                                   NSView* view,
                                                   bool relativeToScreenTop = true)
    {
        NSRect pointInScreenSpaceAsRect;
        pointInScreenSpaceAsRect.size = NSZeroSize;
        pointInScreenSpaceAsRect.origin = pointInScreenSpace;
        if (relativeToScreenTop)
        {
            // The AppKit framework measures y co-ordinates from the bottom of the screen so we must
            // ensure our point is also relative to the bottom before converting it to window space.
            pointInScreenSpaceAsRect.origin.y = NSScreen.mainScreen.frame.size.height - pointInScreenSpace.y;
        }

        // Convert the point into window space then view space
        NSPoint pointInWindowSpace = [view.window convertRectFromScreen: pointInScreenSpaceAsRect].origin;
        NSPoint pointInViewSpace = [view convertPoint: pointInWindowSpace fromView: nil];

        // Make point relative to the top of the view unless it's already using flipped co-ordinates
        if (!view.isFlipped)
        {
            pointInViewSpace.y = view.frame.size.height - pointInViewSpace.y;
        }
        return pointInViewSpace;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Convert point from the local co-ordinate space of an NSView to the global screen co-ordinate
    //! space of the Core Graphics Framework. It is assumed the y co-ordinate of the point passed to
    //! this function is measured from the top-left corner of the NSView, and all necessary flipping
    //! operations will be performed internally to ensure the y co-ordinate of the point returned is
    //! measured from the top-left corner of the screen (as expected by the Core Graphics Framework).
    //! \param[in] pointInViewSpace A point relative to the top-left of a view's co-ordinate space
    //! \param[in] view The NSView whose local co-ordinate space the point will be converted from
    //! \return The point relative to the top-left corner of the screen's global co-ordinate space
    NSPoint ConvertPointFromViewSpaceToScreenSpace(NSPoint pointInViewSpace, NSView* view)
    {
        if (!view.isFlipped)
        {
            // The AppKit framework measures y co-ordinates from the bottom of the screen, and while
            // NSViews can be set to use flipped co-ordinates NSWindows cannot. So before converting
            // it to window space we must ensure our point is relative to the view's top-left corner.
            pointInViewSpace.y = view.frame.size.height - pointInViewSpace.y;
        }

        // Convert the point into window space
        NSRect pointInWindowSpaceAsRect;
        pointInWindowSpaceAsRect.size = NSZeroSize;
        pointInWindowSpaceAsRect.origin = [view convertPoint: pointInViewSpace toView: nil];

        // Convert the point into screen space, then make it relative to the screen's top-left corner
        NSPoint pointInScreenSpace = [view.window convertRectToScreen: pointInWindowSpaceAsRect].origin;
        pointInScreenSpace.y = NSScreen.mainScreen.frame.size.height - pointInScreenSpace.y;
        return pointInScreenSpace;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for Mac mouse input devices
    class InputDeviceMouseMac : public InputDeviceMouse::Implementation
                              , public RawInputNotificationBusMac::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceMouseMac, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceMouseMac(InputDeviceMouse& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceMouseMac() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the event tap that exists while the cursor is constrained
        inline CFMachPortRef GetDisabledSystemCursorEventTap() const { return m_disabledSystemCursorEventTap; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the event tap that exists while the cursor is constrained
        inline void SetDisabledSystemCursorPosition(const CGPoint& point) { m_disabledSystemCursorPosition = point; }

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorState
        void SetSystemCursorState(SystemCursorState systemCursorState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorState
        SystemCursorState GetSystemCursorState() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorPositionNormalized
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorPositionNormalized
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsMac::OnRawInputEvent
        void OnRawInputEvent(const NSEvent* nsEvent) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Update the system cursor visibility
        void UpdateSystemCursorVisibility();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Create an event tap that effectively disables the system cursor, preventing it from
        //! being used to select any other application, but remaining visible and moving around
        //! the screen under user control with the usual system cursor ballistics being applied.
        //! \return True if the event tap was created (or had already been created), false otherwise
        bool CreateDisabledSystemCursorEventTap();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destroy the disabled system cursor event tap
        void DestroyDisabledSystemCursorEventTap();

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        CFRunLoopSourceRef m_disabledSystemCursorRunLoopSource; //!< The disabled cursor run loop
        CFMachPortRef      m_disabledSystemCursorEventTap;      //!< The disabled cursor event tap
        CGPoint            m_disabledSystemCursorPosition;      //!< The disabled cursor position
        SystemCursorState  m_systemCursorState;                 //!< Current system cursor state
        bool               m_hasFocus;                          //!< Does application have focus?
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseMac::InputDeviceMouseMac(InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
        , m_disabledSystemCursorRunLoopSource(nullptr)
        , m_disabledSystemCursorEventTap(nullptr)
        , m_disabledSystemCursorPosition()
        , m_systemCursorState(SystemCursorState::Unknown)
        , m_hasFocus(false)
    {
        RawInputNotificationBusMac::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseMac::~InputDeviceMouseMac()
    {
        RawInputNotificationBusMac::Handler::BusDisconnect();

        // Cleanup system cursor visibility and constraint
        DestroyDisabledSystemCursorEventTap();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouseMac::IsConnected() const
    {
        // If necessary we may be able to determine the connected state using the I/O Kit HIDManager:
        // https://developer.apple.com/library/content/documentation/DeviceDrivers/Conceptual/HID/new_api_10_5/tn2187.html
        //
        // Doing this may allow (and perhaps even force) us to distinguish between multiple physical
        // devices of the same type. But given that support for multiple mice is a fairly niche need
        // we'll keep things simple (for now) and assume there is one (and only one) mouse connected
        // at all times. In practice this means that if multiple physical mice are connected we will
        // process input from them all, but treat all the input as if it comes from the same device.
        //
        // If it becomes necessary to determine the connected state of mouse devices (and/or support
        // distinguishing between multiple physical mice), we should implement this function as well
        // call BroadcastInputDeviceConnectedEvent/BroadcastInputDeviceDisconnectedEvent when needed.
        //
        // Note that doing so will require modifying how we create and manage mouse input devices in
        // InputSystemComponent/InputSystemComponentMac in order to create multiple InputDeviceMouse
        // instances (somehow associating each with a raw mouse device id), along with modifying the
        // InputDeviceMouseMac::OnRawInputEvent function to filter incoming events by raw device id.
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseMac::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        // Because Mac does not provide a way to properly constrain the system cursor inside of a
        // window, when it's constrained we're actually just hiding it and modifying the location
        // of all incoming mouse events so they can't cause the applications window to lose focus.
        // This works fine when the cursor is hidden, provided we normalize its position relative
        // to the entire screen (see InputDeviceMouseMac::GetSystemCursorPositionNormalized). But
        // when the cursor is constrained and visible, we must manually hide the cursor when it's
        // outside the active window (see InputDeviceMouseMac::UpdateSystemCursorVisibility), and
        // we must normalize it relative to the active window or else the position will not match
        // that of the cursor while it is visible inside the active window. Long story short, the
        // ConstrainedAndVisible state on Mac results in a 'dead-zone' where the system is moving
        // the cursor outside of the active window, but the position is clamped within the border
        // of the active window. This is fine when running in full screen on a single monitor but
        // may not provide the greatest user experience in windowed mode or a multi-monitor setup.
        AZ_Warning("InputDeviceMouseMac",
                   systemCursorState != SystemCursorState::ConstrainedAndVisible,
                   "ConstrainedAndVisible does not work entirely as might be expected on Mac when "
                   "running in windowed mode or with a multi-monitor setup. If your game needs to "
                   "support either of these it is recommended you use another system cursor state.");

        if (systemCursorState == m_systemCursorState)
        {
            return;
        }

        m_systemCursorState = systemCursorState;
        UpdateSystemCursorVisibility();
        const bool shouldBeDisabled = (m_systemCursorState == SystemCursorState::ConstrainedAndHidden) ||
                                      (m_systemCursorState == SystemCursorState::ConstrainedAndVisible);

        if (!shouldBeDisabled)
        {
            DestroyDisabledSystemCursorEventTap();
            return;
        }

        const bool isDisabled = CreateDisabledSystemCursorEventTap();
        if (!isDisabled)
        {
            AZ_Warning("InputDeviceMouseMac", false, "Failed create event tap; cursor cannot be constrained as requested");
            m_systemCursorState = SystemCursorState::UnconstrainedAndVisible;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SystemCursorState InputDeviceMouseMac::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseMac::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        CGPoint newPositionInScreenSpace;
        if (m_systemCursorState == SystemCursorState::ConstrainedAndHidden)
        {
            // Because Mac does not provide a way to properly constrain the system cursor inside of
            // a window, when it is constrained we are actually just modifying the locations of all
            // incoming mouse events so they cannot cause the application window to lose focus. So
            // if the cursor is also hidden we simply need to de-normalize the position relative to
            // the entire screen. See comment in InputDeviceMouseMac::SetSystemCursorState for more.
            NSSize cursorFrameSize = NSScreen.mainScreen.frame.size;
            newPositionInScreenSpace.x = positionNormalized.GetX() * cursorFrameSize.width;
            newPositionInScreenSpace.y = positionNormalized.GetY() * cursorFrameSize.height;
            m_disabledSystemCursorPosition = newPositionInScreenSpace;
        }
        else
        {
            // However, if the system cursor is visible or not constrained, we need to de-normalize
            // the desired position relative to the content rect of the application's main view and
            // then convert it to screen space.
            NSView* mainView = GetSystemCursorMainContentView();
            NSSize cursorFrameSize = mainView.frame.size;
            NSPoint newPositionInViewSpace = NSMakePoint(positionNormalized.GetX() * cursorFrameSize.width,
                                                         positionNormalized.GetY() * cursorFrameSize.height);
            newPositionInScreenSpace = ConvertPointFromViewSpaceToScreenSpace(newPositionInViewSpace, mainView);
        }

        CGWarpMouseCursorPosition(newPositionInScreenSpace);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceMouseMac::GetSystemCursorPositionNormalized() const
    {
        // Because Mac does not provide a way to properly constrain the system cursor inside of a
        // window, when it's constrained we are actually just disabling it by modifying locations
        // of all incoming mouse events so they cannot cause the application window to lose focus.
        //
        // So if the cursor is also hidden we simply need to normalize the position relative to the
        // entire screen, but we must use the disabled position because calling CGEventSetLocation
        // (see DisabledSysetmCursorEventTapCallback) results in NSEvent.mouseLocation returning
        // the clamped position (along with CGEventGetLocation and CGEventGetUnflippedLocation).
        NSSize cursorFrameSize = NSScreen.mainScreen.frame.size;
        NSPoint cursorPosition = m_disabledSystemCursorPosition;

        if (m_systemCursorState != SystemCursorState::ConstrainedAndHidden)
        {
            // However, if the system cursor is visible or not constrained, we need to normalize
            // the cursor position (which in this case can be obtained by NSEvent.mouseLocation)
            // relative to the content rect of the application's main view.
            NSView* mainView = GetSystemCursorMainContentView();
            cursorFrameSize = mainView.frame.size;
            cursorPosition = ConvertPointFromScreenSpaceToViewSpace(NSEvent.mouseLocation,
                                                                    mainView,
                                                                    false);
        }

        // Normalize the cursor position
        const float cursorPostionNormalizedX = cursorFrameSize.width != 0.0f ? cursorPosition.x / cursorFrameSize.width : 0.0f;
        const float cursorPostionNormalizedY = cursorFrameSize.height != 0.0f ? cursorPosition.y / cursorFrameSize.height : 0.0f;

        return AZ::Vector2(cursorPostionNormalizedX, cursorPostionNormalizedY);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseMac::TickInputDevice()
    {
        // The event loop has just been pumped in ApplicationRequests::PumpSystemEventLoopUntilEmpty,
        // so we now just need to process any raw events that have been queued since the last frame
        const bool hadFocus = m_hasFocus;
        m_hasFocus = NSApplication.sharedApplication.active;
        if (m_hasFocus)
        {
            // Update the visibility of the system cursor, which unfortunately must be done every
            // frame to combat the cursor being displayed by the system under some circumstances.
            UpdateSystemCursorVisibility();

            // Process raw event queues once each frame while this application's window has focus
            ProcessRawEventQueues();
        }
        else if (hadFocus)
        {
            // This application's window no longer has focus, process any events that are queued,
            // before resetting the state of all this input device's associated input channels.
            ProcessRawEventQueues();
            ResetInputChannelStates();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseMac::OnRawInputEvent(const NSEvent* nsEvent)
    {
        switch (nsEvent.type)
        {
            // Left button
            case NSEventTypeLeftMouseDown:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Left, true);
            }
            break;
            case NSEventTypeLeftMouseUp:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Left, false);
            }
            break;

            // Right button
            case NSEventTypeRightMouseDown:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Right, true);
            }
            break;
            case NSEventTypeRightMouseUp:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Right, false);
            }
            break;

            // Middle button
            case NSEventTypeOtherMouseDown:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Middle, true);
            }
            break;
            case NSEventTypeOtherMouseUp:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Middle, false);
            }
            break;

            // Scroll wheel
            case NSEventTypeScrollWheel:
            {
                QueueRawMovementEvent(InputDeviceMouse::Movement::Z, nsEvent.scrollingDeltaY);
            }
            break;

            // Mouse movement
            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged:
            case NSEventTypeRightMouseDragged:
            case NSEventTypeOtherMouseDragged:
            {
                QueueRawMovementEvent(InputDeviceMouse::Movement::X, nsEvent.deltaX);
                QueueRawMovementEvent(InputDeviceMouse::Movement::Y, nsEvent.deltaY);
            }
            break;

            default:
            {
                // Ignore
            }
            break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsPointInsideMainView(const NSPoint& pointInScreenSpace, bool relativeToTopLeft = true)
    {
        NSView* mainView = GetSystemCursorMainContentView();
        NSPoint pointInViewSpace = ConvertPointFromScreenSpaceToViewSpace(pointInScreenSpace,
                                                                          mainView,
                                                                          relativeToTopLeft);
        return NSPointInRect(pointInViewSpace, mainView.bounds);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseMac::UpdateSystemCursorVisibility()
    {
        bool shouldCursorBeVisible = true;
        switch (m_systemCursorState)
        {
            case SystemCursorState::ConstrainedAndHidden:
            {
                shouldCursorBeVisible = false;
            }
            break;
            case SystemCursorState::ConstrainedAndVisible:
            {
                shouldCursorBeVisible = IsPointInsideMainView(m_disabledSystemCursorPosition);
            }
            break;
            case SystemCursorState::UnconstrainedAndHidden:
            {
                shouldCursorBeVisible = !IsPointInsideMainView(NSEvent.mouseLocation, false);
            }
            break;
            case SystemCursorState::UnconstrainedAndVisible:
            case SystemCursorState::Unknown:
            {
                shouldCursorBeVisible = true;
            }
            break;
        }

        // CGCursorIsVisible has been deprecated but still works, and there is no other way to check
        // whether the system cursor is currently visible. Calls to CGDisplayHideCursor are meant to
        // increment an application specific 'cursor hidden' counter that must be balanced by a call
        // to CGDisplayShowCursor, however this doesn't seem to work if the cursor is shown again by
        // the system (or another application), which can happen when this application loses focus.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        const bool isCursorVisible = CGCursorIsVisible();
#pragma clang diagnostic pop
        if (isCursorVisible && !shouldCursorBeVisible)
        {
            CGDisplayHideCursor(kCGNullDirectDisplay);
        }
        else if (!isCursorVisible && shouldCursorBeVisible)
        {
            CGDisplayShowCursor(kCGNullDirectDisplay);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    CGEventRef DisabledSysetmCursorEventTapCallback(CGEventTapProxy eventTapProxy,
                                                       CGEventType eventType,
                                                       CGEventRef eventRef,
                                                       void* userInfo)
    {
        InputDeviceMouseMac* inputDeviceMouse = static_cast<InputDeviceMouseMac*>(userInfo);
        switch (eventType)
        {
            case kCGEventTapDisabledByTimeout:
            case kCGEventTapDisabledByUserInput:
            {
                // Re-enable the event tap if it gets disabled (important to do this first)
                CGEventTapEnable(inputDeviceMouse->GetDisabledSystemCursorEventTap(), true);
                return eventRef;
            }
            break;
        }

        if (!NSApplication.sharedApplication.active)
        {
            // Do nothing if the application is not active
            return eventRef;
        }

        NSView* mainView = GetSystemCursorMainContentView();
        if (mainView == nil || mainView.window == nil)
        {
            // Do nothing if we don't have a main view
            return eventRef;
        }

        // Store the location of the event before it is (potentially) modified below using
        // CGEventSetLocation. This is needed so that the un-clamped position can still be
        // used to get/set the system cursor position while it is ConstrainedAndHidden.
        CGPoint eventLocationRelativeToTopLeft = CGEventGetLocation(eventRef);
        inputDeviceMouse->SetDisabledSystemCursorPosition(eventLocationRelativeToTopLeft);

        // Get the location of the event relative to the bottom left of the screen, needed
        // so that it can be checked against the cursor bounds from this co-ordinate space.
        CGPoint eventLocation = CGEventGetUnflippedLocation(eventRef);

        // Get the current cursor bounds of the main view, then adjust them to exclude the
        // corner radius, whose value was deduced by trial and error because there doesn't
        // appear to be any other way to get it (mainView.layer.cornerRadius returns 0.0f).
        NSRect cursorBoundsInWindowSpace = [mainView convertRect: mainView.bounds toView: nil];
        NSRect cursorBounds = [mainView.window convertRectToScreen: cursorBoundsInWindowSpace];
        const float cornerRadius = 2.0f;
        cursorBounds = NSInsetRect(cursorBounds, cornerRadius, cornerRadius);

        if (NSPointInRect(NSPointFromCGPoint(eventLocation), cursorBounds))
        {
            // Do nothing if the event occured inside of the application's main view
            return eventRef;
        }

        // Constrain the event location to the cursor bounds.
        eventLocation.x = AZ::GetClamp(eventLocation.x, NSMinX(cursorBounds), NSMaxX(cursorBounds));
        eventLocation.y = AZ::GetClamp(eventLocation.y, NSMinY(cursorBounds), NSMaxY(cursorBounds));

        // Reset the event location after flipping it back to be relative to the top of the screen
        eventLocation.y = NSMaxY(NSScreen.mainScreen.frame) - eventLocation.y;
        CGEventSetLocation(eventRef, eventLocation);

        return eventRef;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouseMac::CreateDisabledSystemCursorEventTap()
    {
        if (m_disabledSystemCursorEventTap != nullptr)
        {
            // It has already been created
            return true;
        }

        // Create an event tap that listens for all mouse events
        static const CGEventMask allMouseEventsMask = CGEventMaskBit(kCGEventLeftMouseDown) |
                                                      CGEventMaskBit(kCGEventLeftMouseDragged) |
                                                      CGEventMaskBit(kCGEventLeftMouseUp) |
                                                      CGEventMaskBit(kCGEventRightMouseDown) |
                                                      CGEventMaskBit(kCGEventRightMouseDragged) |
                                                      CGEventMaskBit(kCGEventRightMouseUp) |
                                                      CGEventMaskBit(kCGEventOtherMouseDown) |
                                                      CGEventMaskBit(kCGEventOtherMouseUp) |
                                                      CGEventMaskBit(kCGEventMouseMoved);
        m_disabledSystemCursorEventTap = CGEventTapCreate(kCGSessionEventTap,
                                                          kCGHeadInsertEventTap,
                                                          kCGEventTapOptionDefault,
                                                          allMouseEventsMask,
                                                          &DisabledSysetmCursorEventTapCallback,
                                                          this);
        if (m_disabledSystemCursorEventTap == nullptr)
        {
            return false;
        }

        // Create a run loop source
        m_disabledSystemCursorRunLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_disabledSystemCursorEventTap, 0);
        if (m_disabledSystemCursorRunLoopSource == nullptr)
        {
            CFRelease(m_disabledSystemCursorEventTap);
            m_disabledSystemCursorEventTap = nullptr;
            return false;
        }

        // Add the run loop source and enable the event tap
        CFRunLoopAddSource(CFRunLoopGetCurrent(), m_disabledSystemCursorRunLoopSource, kCFRunLoopCommonModes);
        CGEventTapEnable(m_disabledSystemCursorEventTap, true);

        // Initialize the constrained system cursor position
        CGEventRef event = CGEventCreate(nil);
        m_disabledSystemCursorPosition = CGEventGetLocation(event);
        CFRelease(event);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseMac::DestroyDisabledSystemCursorEventTap()
    {
        if (m_disabledSystemCursorEventTap == nullptr)
        {
            // It has already been destroyed
            return;
        }

        // Disable the event tap and remove the run loop source
        CGEventTapEnable(m_disabledSystemCursorEventTap, false);
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), m_disabledSystemCursorRunLoopSource, kCFRunLoopCommonModes);

        // Destroy the run loop source
        CFRelease(m_disabledSystemCursorRunLoopSource);
        m_disabledSystemCursorRunLoopSource = nullptr;

        // Destroy the event tap
        CFRelease(m_disabledSystemCursorEventTap);
        m_disabledSystemCursorEventTap = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class MacDeviceMouseImplFactory
        : public InputDeviceMouse::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceMouse::Implementation> Create(InputDeviceMouse& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceMouseMac>(inputDevice);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeDeviceMouseImplentationFactory()
    {
        m_deviceMouseImplFactory = AZStd::make_unique<MacDeviceMouseImplFactory>();
        AZ::Interface<InputDeviceMouse::ImplementationFactory>::Register(m_deviceMouseImplFactory.get());
    }
} // namespace AzFramework
