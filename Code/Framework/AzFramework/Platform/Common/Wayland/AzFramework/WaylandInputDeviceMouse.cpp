/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WaylandInputDeviceMouse.h"

#include <AzCore/Console/IConsole.h>

#include <AzFramework/Protocols/SeatManager.h>
#include <AzFramework/WaylandNativeWindow.h>

#include <linux/input-event-codes.h>

#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "WaylandProtocolNames.h"

namespace AzFramework
{
    // Using the accelerated values should be the default for relative pointer
    AZ_CVAR(
        bool,
        wl_accel,
        1,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "WAYLAND ONLY: Set to use accelerated values, this only works if the compositor supports relative pointer.");

    wl_pointer_listener WaylandInputDeviceMouse::s_pointerListener = {
        .enter = PointerEnter,
        .leave = PointerLeave,
        .motion = PointerMotion,
        .button = PointerButton,
        .axis = PointerAxis,
        .frame = PointerFrame,
        .axis_source = PointerAxisSource,
        .axis_stop = PointerAxisStop,
        .axis_discrete = PointerAxisDiscrete,
        .axis_value120 = PointerAxisValue120,
        .axis_relative_direction = PointerAxisRelDir
    };

    zwp_relative_pointer_v1_listener WaylandInputDeviceMouse::s_relPointerListener = {
        .relative_motion = RelPointerMotion
    };

    void WaylandInputDeviceMouse::PointerEnter(
        void* data,
        wl_pointer* /*wl_pointer*/,
        uint32_t serial,
        wl_surface* surface,
        wl_fixed_t surface_x,
        wl_fixed_t surface_y)
    {
        auto self = static_cast<WaylandInputDeviceMouse*>(data);
        self->m_focusedWindow = static_cast<NativeWindowHandle>(surface);
        self->m_currentSerial = serial;

        self->m_axisEvent.m_eventMask |= PointerEventMask::POINTER_EVENT_ENTER;
        self->m_axisEvent.m_serial = serial;
        self->m_axisEvent.m_surfaceX = surface_x;
        self->m_axisEvent.m_surfaceY = surface_y;

        self->InternalApplyCursorState();
    }

    void WaylandInputDeviceMouse::PointerLeave(void* data, wl_pointer* /*wl_pointer*/, uint32_t serial,
                                               wl_surface* surface)
    {
        auto self = static_cast<WaylandInputDeviceMouse*>(data);
        self->m_focusedWindow = nullptr;

        // I don't really see a need to save the pointer leave serial
        // If you do, please update the code that checks to see if currentSerial is at MAX since
        // those assume if it's not at MAX then the pointer has entered.
        self->m_currentSerial = UINT32_MAX;

        self->m_axisEvent.m_eventMask |= PointerEventMask::POINTER_EVENT_LEAVE;
        self->m_axisEvent.m_serial = serial;
    }

    void WaylandInputDeviceMouse::PointerMotion(
        void* data, wl_pointer* /*wl_pointer*/, uint32_t /*time*/, wl_fixed_t surface_x, wl_fixed_t surface_y)
    {
        auto self = static_cast<WaylandInputDeviceMouse*>(data);
        self->m_position = AZ::Vector2((float)wl_fixed_to_double(surface_x), (float)wl_fixed_to_double(surface_y));
        if (self->m_focusedWindow)
        {
            float dpiScaleFactor = 1.0f;
            WindowRequestBus::EventResult(dpiScaleFactor, self->m_focusedWindow, &WindowRequests::GetDpiScaleFactor);
            self->m_position *= dpiScaleFactor;
        }
    }

    void WaylandInputDeviceMouse::PointerButton(
        void* data, wl_pointer* /*wl_pointer*/, uint32_t /*serial*/, uint32_t /*time*/, uint32_t button,
        uint32_t state)
    {
        auto self = static_cast<WaylandInputDeviceMouse*>(data);
        bool pressed = state & wl_pointer_button_state::WL_POINTER_BUTTON_STATE_PRESSED;

        switch (button)
        {
        case BTN_LEFT:
            self->QueueRawButtonEvent(InputDeviceMouse::Button::Left, pressed);
            break;
        case BTN_RIGHT:
            self->QueueRawButtonEvent(InputDeviceMouse::Button::Right, pressed);
            break;
        case BTN_MIDDLE:
            self->QueueRawButtonEvent(InputDeviceMouse::Button::Middle, pressed);
            break;
        case BTN_SIDE:
            self->QueueRawButtonEvent(InputDeviceMouse::Button::Other1, pressed);
            break;
        case BTN_EXTRA:
            self->QueueRawButtonEvent(InputDeviceMouse::Button::Other2, pressed);
            break;
        default:
            break;
        }
    }

    void WaylandInputDeviceMouse::PointerAxis(void* data, wl_pointer* /*wl_pointer*/, uint32_t time,
                                              uint32_t axis, wl_fixed_t value)
    {
        auto self = static_cast<WaylandInputDeviceMouse*>(data);
        self->m_axisEvent.m_eventMask |= POINTER_EVENT_AXIS;
        self->m_axisEvent.m_time = time;
        self->m_axisEvent.m_axis[axis].m_valid = true;
        self->m_axisEvent.m_axis[axis].m_value = value;
    }

    void WaylandInputDeviceMouse::PointerAxisSource(void* data, wl_pointer* /*wl_pointer*/, uint32_t axis_source)
    {
        auto self = static_cast<WaylandInputDeviceMouse*>(data);
        self->m_axisEvent.m_eventMask |= POINTER_EVENT_AXIS_SOURCE;
        self->m_axisEvent.m_axisSource = axis_source;
    }

    void WaylandInputDeviceMouse::PointerAxisStop(void* data, wl_pointer* /*wl_pointer*/, uint32_t time,
                                                  uint32_t axis)
    {
        auto self = static_cast<WaylandInputDeviceMouse*>(data);
        self->m_axisEvent.m_time = time;
        self->m_axisEvent.m_eventMask |= POINTER_EVENT_AXIS_STOP;
        self->m_axisEvent.m_axis[axis].m_valid = true;
    }

    void WaylandInputDeviceMouse::PointerAxisDiscrete(void* data, wl_pointer* /*wl_pointer*/, uint32_t axis,
                                                      int32_t discrete)
    {
        auto self = static_cast<WaylandInputDeviceMouse*>(data);
        self->m_axisEvent.m_eventMask |= POINTER_EVENT_AXIS_DISCRETE;
        self->m_axisEvent.m_axis[axis].m_valid = true;
        self->m_axisEvent.m_axis[axis].m_discrete = discrete;
    }

    void WaylandInputDeviceMouse::PointerAxisValue120(
        void* /*data*/, wl_pointer* /*wl_pointer*/, uint32_t /*axis*/, int32_t /*value120*/)
    {
    }

    void WaylandInputDeviceMouse::PointerAxisRelDir(
        void* /*data*/, wl_pointer* /*wl_pointer*/, uint32_t /*axis*/, uint32_t /*direction*/)
    {
    }

    void WaylandInputDeviceMouse::PointerFrame(void* data, wl_pointer*)
    {
        auto self = static_cast<WaylandInputDeviceMouse*>(data);

        if (self->m_axisEvent.m_eventMask & POINTER_EVENT_AXIS)
        {
            const auto& verticalScrollEvent = self->m_axisEvent.m_axis[WL_POINTER_AXIS_VERTICAL_SCROLL];
            if (verticalScrollEvent.m_valid)
            {
                self->QueueRawMovementEvent(InputDeviceMouse::Movement::Z,
                                            -(float)wl_fixed_to_double(verticalScrollEvent.m_value) * 8.0f);
            }
        }

        self->m_axisEvent = {};
    }

    void WaylandInputDeviceMouse::RelPointerMotion(
        void* data,
        zwp_relative_pointer_v1*,
        uint32_t,
        uint32_t,
        wl_fixed_t dx,
        wl_fixed_t dy,
        wl_fixed_t dx_unaccel,
        wl_fixed_t dy_unaccel)
    {
        auto self = static_cast<WaylandInputDeviceMouse*>(data);

        wl_fixed_t x = dx_unaccel;
        wl_fixed_t y = dy_unaccel;

        if (wl_accel)
        {
            x = dx;
            y = dy;
        }

        if (x != 0)
        {
            self->QueueRawMovementEvent(InputDeviceMouse::Movement::X, (float)wl_fixed_to_double(x));
        }
        if (y != 0)
        {
            self->QueueRawMovementEvent(InputDeviceMouse::Movement::Y, (float)wl_fixed_to_double(y));
        }
    }

    WaylandInputDeviceMouse::WaylandInputDeviceMouse(AzFramework::InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
          , m_playerIdx(inputDevice.GetInputDeviceId().GetIndex())
          , m_cursorState(SystemCursorState::UnconstrainedAndVisible)
    {
        SeatCapsChanged();
        SeatNotificationsBus::Handler::BusConnect(m_playerIdx);

        if (auto wl = WaylandConnectionManagerInterface::Get())
        {
            m_confinedRegion = wl_compositor_create_region(wl->GetWaylandCompositor());
        }
    }

    WaylandInputDeviceMouse::~WaylandInputDeviceMouse()
    {
        SeatNotificationsBus::Handler::BusDisconnect();
        UpdatePointer(nullptr);
    }

    WaylandInputDeviceMouse::Implementation* WaylandInputDeviceMouse::Create(AzFramework::InputDeviceMouse& inputDevice)
    {
        return aznew WaylandInputDeviceMouse(inputDevice);
    }

    void WaylandInputDeviceMouse::UpdatePointer(wl_pointer* newPointer)
    {
        if (newPointer == m_pointer)
        {
            return;
        }

        if (m_pointer != nullptr)
        {
            if (auto cursorManager = CursorShapeInterface::Get())
            {
                cursorManager->UnregisterPointer(m_pointer);
            }
            if (m_relPointer != nullptr)
            {
                zwp_relative_pointer_v1_destroy(m_relPointer);
                m_relPointer = nullptr;
            }
            wl_pointer_release(m_pointer);
        }

        m_pointer = newPointer;
        if (m_pointer)
        {
            wl_pointer_add_listener(m_pointer, &s_pointerListener, this);

            if (auto cursorManager = CursorShapeInterface::Get())
            {
                cursorManager->RegisterPointer(m_pointer);
            }
            if (auto relManager = RelativePointerInterface::Get())
            {
                m_relPointer = relManager->GetRelativePointer(m_pointer);
                if (m_relPointer != nullptr)
                {
                    zwp_relative_pointer_v1_set_user_data(m_relPointer, this);
                    zwp_relative_pointer_v1_add_listener(m_relPointer, &s_relPointerListener, this);
                }
            }
        }
    }

    void WaylandInputDeviceMouse::ReleaseSeat()
    {
        UpdatePointer(nullptr);
    }

    void WaylandInputDeviceMouse::SeatCapsChanged()
    {
        const auto* interface = AzFramework::SeatManagerInterface::Get();
        if (!interface)
        {
            return;
        }

        UpdatePointer(interface->GetSeatPointer(m_playerIdx));
    }

    bool WaylandInputDeviceMouse::IsConnected() const
    {
        return m_pointer != nullptr;
    }

    void WaylandInputDeviceMouse::SetSystemCursorState(AzFramework::SystemCursorState systemCursorState)
    {
        m_cursorState = systemCursorState;

        // We shouldn't call ApplyCursorState unless the pointer entered a window
        if (m_currentSerial != UINT32_MAX)
        {
            InternalApplyCursorState();
        }
    }

    SystemCursorState WaylandInputDeviceMouse::GetSystemCursorState() const
    {
        return m_cursorState;
    }

    void WaylandInputDeviceMouse::SetSystemCursorPositionNormalized(AZ::Vector2)
    {
        // We can do this when locked but only as a hint to the compositor
        // that we want the cursor to show up at that location when unlocked.
        // I think O3DE uses this to warp the cursor somewhere when unlocked, which won't work on Wayland by design.
    }

    AZ::Vector2 WaylandInputDeviceMouse::GetSystemCursorPositionNormalized() const
    {
        if (m_focusedWindow)
        {
            WindowSize windowSize = {};
            WindowRequestBus::EventResult(windowSize, m_focusedWindow, &WindowRequests::GetClientAreaSize);
            return m_position / AZ::Vector2((float)windowSize.m_width, (float)windowSize.m_height);
        }

        return m_position;
    }

    void WaylandInputDeviceMouse::TickInputDevice()
    {
        ProcessRawEventQueues();
    }

    void WaylandInputDeviceMouse::InternalApplyCursorState()
    {
        if (m_currentSerial == UINT32_MAX)
        {
            // Need a serial code
            AZ_Error("WaylandInputDeviceMouse", false, "Calling InternalApplyCursorState but no serial");
            return;
        }
        switch (m_cursorState)
        {
        case SystemCursorState::ConstrainedAndHidden:
            InternalSetShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT, false);
            InternalConstrainMouse(true);
            break;

        case SystemCursorState::ConstrainedAndVisible:
            InternalSetShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT, true);
            InternalConstrainMouse(true);
            break;

        case SystemCursorState::UnconstrainedAndVisible:
            InternalSetShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT, true);
            InternalConstrainMouse(false);
            break;

        case SystemCursorState::UnconstrainedAndHidden:
            InternalSetShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT, false);
            InternalConstrainMouse(false);
            break;
        default:
            InternalSetShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT, true);
            InternalConstrainMouse(false);
            break;
        }
    }

    void WaylandInputDeviceMouse::InternalSetShape(const wp_cursor_shape_device_v1_shape shape, bool visible)
    {
        if (m_currentSerial == UINT32_MAX)
        {
            AZ_Error("WaylandInputDeviceMouse", false, "Calling InternalSetShape but no serial");
            return;
        }

        if (m_pointer == nullptr)
        {
            return;
        }

        if (!visible)
        {
            wl_pointer_set_cursor(m_pointer, m_currentSerial, nullptr, 0, 0);
            return;
        }

        if (auto cursorManager = CursorShapeInterface::Get())
        {
            cursorManager->SetCursorShape(m_pointer, m_currentSerial, shape);
        }
    }

    void WaylandInputDeviceMouse::InternalConstrainMouse(bool wantConstraints)
    {
        if (m_pointer == nullptr)
        {
            return;
        }

        if (wantConstraints)
        {
            if (m_lockedPointer != nullptr)
            {
                // Already confined
                return;
            }

            if (m_focusedWindow == nullptr)
            {
                // No focused window.
                return;
            }

            wl_proxy* proxy = nullptr;
            WaylandProxyBus::EventResult(proxy, PointerConstraintsName, &WaylandProxyBus::Events::GetProxy,
                                         PointerConstraintsName);
            auto constraints = reinterpret_cast<zwp_pointer_constraints_v1*>(proxy);
            if (constraints)
            {
                NativeWindowHandle constraintWindow = nullptr;
                InputSystemCursorConstraintRequestBus::BroadcastResult(
                    constraintWindow,
                    &InputSystemCursorConstraintRequestBus::Events::GetSystemCursorConstraintWindow);

                if (constraintWindow == nullptr)
                {
                    // Use focused
                    constraintWindow = m_focusedWindow;
                }

                // Remove our old region
                wl_region_subtract(
                    m_confinedRegion,
                    (int32_t)m_currentRegion.m_posX,
                    (int32_t)m_currentRegion.m_posY,
                    (int32_t)m_currentRegion.m_width,
                    (int32_t)m_currentRegion.m_height);

                WindowSize constraintSize = {};
                WindowRequestBus::EventResult(constraintSize, constraintWindow, &WindowRequests::GetClientAreaSize);

                m_currentRegion = {0, 0, constraintSize.m_width, constraintSize.m_height};
                wl_region_add(
                    m_confinedRegion,
                    (int32_t)m_currentRegion.m_posX,
                    (int32_t)m_currentRegion.m_posY,
                    (int32_t)m_currentRegion.m_width,
                    (int32_t)m_currentRegion.m_height);

                m_lockedPointer = zwp_pointer_constraints_v1_lock_pointer(
                    constraints,
                    static_cast<wl_surface*>(constraintWindow),
                    m_pointer,
                    m_confinedRegion,
                    ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
            }
        }
        else
        {
            if (m_lockedPointer)
            {
                // Don't want constraints anymore
                zwp_locked_pointer_v1_destroy(m_lockedPointer);
                m_lockedPointer = nullptr;
            }
        }
    }
} // namespace AzFramework
