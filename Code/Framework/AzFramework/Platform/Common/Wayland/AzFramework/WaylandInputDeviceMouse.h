/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Protocols/CursorShapeInterface.h>
#include <AzFramework/Protocols/RelativePointerInterface.h>
#include <AzFramework/Protocols/SeatManager.h>
#include <AzFramework/WaylandConnectionManager.h>
#include <AzFramework/WaylandInterface.h>

#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>

struct zwp_locked_pointer_v1;

namespace AzFramework
{
    class WaylandInputDeviceMouse final
        : public InputDeviceMouse::Implementation
        , public SeatNotificationsBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(WaylandInputDeviceMouse, AZ::SystemAllocator);

        WaylandInputDeviceMouse(InputDeviceMouse& inputDevice);

        virtual ~WaylandInputDeviceMouse();

        static WaylandInputDeviceMouse::Implementation* Create(InputDeviceMouse& inputDevice);

        void UpdatePointer(wl_pointer* newPointer);
        void ReleaseSeat() override;
        void SeatCapsChanged() override;

        bool IsConnected() const override;
        void SetSystemCursorState(AzFramework::SystemCursorState systemCursorState) override;
        SystemCursorState GetSystemCursorState() const override;
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;
        void TickInputDevice() override;

        // You should make sure the cursor is in the entered state before calling.
        void InternalApplyCursorState();
        void InternalSetShape(const wp_cursor_shape_device_v1_shape shape, bool visible);
        void InternalConstrainMouse(bool wantConstraints);

    private:
        static void PointerEnter(
            void* data,
            wl_pointer* wl_pointer,
            uint32_t serial,
            wl_surface* surface,
            wl_fixed_t surface_x,
            wl_fixed_t surface_y);

        static void PointerLeave(void* data, wl_pointer* wl_pointer, uint32_t serial, wl_surface* surface);
        static void PointerMotion(void* data, wl_pointer* wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);

        static void PointerButton(
            void* data, wl_pointer* wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);

        static void PointerAxis(void* data, wl_pointer* wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
        static void PointerFrame(void* data, wl_pointer* wl_pointer);
        static void PointerAxisSource(void* data, wl_pointer* wl_pointer, uint32_t axis_source);
        static void PointerAxisStop(void* data, wl_pointer* wl_pointer, uint32_t time, uint32_t axis);
        static void PointerAxisDiscrete(void* data, wl_pointer* wl_pointer, uint32_t axis, int32_t discrete);
        static void PointerAxisValue120(void* data, wl_pointer* wl_pointer, uint32_t axis, int32_t value120);
        static void PointerAxisRelDir(void* data, wl_pointer* wl_pointer, uint32_t axis, uint32_t direction);

        static wl_pointer_listener s_pointerListener;

        static void RelPointerMotion(
            void* data,
            zwp_relative_pointer_v1* zwp_relative_pointer_v1,
            uint32_t utime_hi,
            uint32_t utime_lo,
            wl_fixed_t dx,
            wl_fixed_t dy,
            wl_fixed_t dx_unaccel,
            wl_fixed_t dy_unaccel);

        static zwp_relative_pointer_v1_listener s_relPointerListener;

    protected:
        uint32_t m_playerIdx = 0;
        wl_pointer* m_pointer = nullptr;
        zwp_relative_pointer_v1* m_relPointer = nullptr;
        zwp_locked_pointer_v1* m_lockedPointer = nullptr;

        wl_region* m_confinedRegion = nullptr;
        WindowGeometry m_currentRegion = { 0, 0, 0, 0 };

        uint32_t m_currentSerial = UINT32_MAX;
        AzFramework::SystemCursorState m_cursorState = SystemCursorState::Unknown;
        NativeWindowHandle m_focusedWindow = nullptr;

        AZ::Vector2 m_position = AZ::Vector2::CreateZero();

        // Pointer axis event
        enum PointerEventMask : uint32_t
        {
            POINTER_EVENT_ENTER = 1 << 0,
            POINTER_EVENT_LEAVE = 1 << 1,
            POINTER_EVENT_MOTION = 1 << 2,
            POINTER_EVENT_BUTTON = 1 << 3,
            POINTER_EVENT_AXIS = 1 << 4,
            POINTER_EVENT_AXIS_SOURCE = 1 << 5,
            POINTER_EVENT_AXIS_STOP = 1 << 6,
            POINTER_EVENT_AXIS_DISCRETE = 1 << 7,
        };
        struct
        {
            uint32_t m_eventMask;
            wl_fixed_t m_surfaceX, m_surfaceY;
            uint32_t m_button, m_state;
            uint32_t m_time;
            uint32_t m_serial;
            struct
            {
                bool m_valid;
                wl_fixed_t m_value;
                int32_t m_discrete;
            } m_axis[2];
            uint32_t m_axisSource;
        } m_axisEvent = {};
    };
} // namespace AzFramework
