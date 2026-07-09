/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Time/TimeSystem.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Protocols/SeatManager.h>
#include <AzFramework/WaylandConnectionManager.h>
#include <AzFramework/Windowing/WindowBus.h>

#include <xkbcommon/xkbcommon.h>

namespace AzFramework
{
    class WaylandInputDeviceKeyboard
        : public InputDeviceKeyboard::Implementation
        , public SeatNotificationsBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(WaylandInputDeviceKeyboard, AZ::SystemAllocator);

        using InputDeviceKeyboard::Implementation::Implementation;
        WaylandInputDeviceKeyboard(InputDeviceKeyboard& inputDevice);

        ~WaylandInputDeviceKeyboard() override;

        static WaylandInputDeviceKeyboard::Implementation* Create(InputDeviceKeyboard& inputDevice);

        void UpdateKeyboard(wl_keyboard* newKeyboard);
        void ReleaseSeat() override;
        void SeatCapsChanged() override;

        bool IsConnected() const override;

        bool HasTextEntryStarted() const override;
        void TextEntryStart(const AzFramework::InputTextEntryRequests::VirtualKeyboardOptions& options) override;
        void TextEntryStop() override;

        void ResetRepeatState();

        void TickInputDevice() override;

    private:
        static void KeyboardEnter(
            void* data, wl_keyboard* wl_keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);

        static void KeyboardLeave(void* data, wl_keyboard* wl_keyboard, uint32_t serial, wl_surface* surface);
        static void KeyboardKeymap(void* data, wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size);
        static void KeyboardKey(void* data, wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);

        static void KeyboardModifiers(
            void* data,
            wl_keyboard* wl_keyboard,
            uint32_t serial,
            uint32_t mods_depressed,
            uint32_t mods_latched,
            uint32_t mods_locked,
            uint32_t group);

        static void KeyboardRepeatInfo(void* data, wl_keyboard* wl_keyboard, int32_t rate, int32_t delay);

        static wl_keyboard_listener s_keyboardListener;

        void SendKeyEvent(uint32_t waylandKey, bool isPressed);
        const InputChannelId* InputChannelFromKeySym(xkb_keysym_t keysym);
        AZStd::string TextFromKeyCode(xkb_keycode_t code);

    protected:
        uint32_t m_playerIdx = 0;
        wl_keyboard* m_keyboard = nullptr;
        xkb_state* m_xkbState = nullptr;
        xkb_context* m_xkbContext = nullptr;
        xkb_keymap* m_xkbKeymap = nullptr;
        uint32_t m_currentSerial = UINT32_MAX;

        int32_t m_repeatDelayMs = 0;
        int32_t m_repeatRatePerSec = 0;
        AZStd::string m_currentHeldKey;
        AZ::TimeMs m_heldKeyElapsed;
        AZ::TimeMs m_lastRepeatTime;

        bool m_inTextMode = false;

        NativeWindowHandle m_focusedWindow = nullptr;
    };
} // namespace AzFramework
