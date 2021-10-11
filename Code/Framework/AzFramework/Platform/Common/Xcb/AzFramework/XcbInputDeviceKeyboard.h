/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/XcbEventHandler.h>
#include <AzFramework/XcbInterface.h>

#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>

struct xcb_xkb_state_notify_event_t;

namespace AzFramework
{
    class XcbInputDeviceKeyboard
        : public InputDeviceKeyboard::Implementation
        , public XcbEventHandlerBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(XcbInputDeviceKeyboard, AZ::SystemAllocator, 0);

        using InputDeviceKeyboard::Implementation::Implementation;
        XcbInputDeviceKeyboard(InputDeviceKeyboard& inputDevice);

        bool IsConnected() const override;

        bool HasTextEntryStarted() const override;
        void TextEntryStart(const InputDeviceKeyboard::VirtualKeyboardOptions& options) override;
        void TextEntryStop() override;
        void TickInputDevice() override;

        void HandleXcbEvent(xcb_generic_event_t* event) override;

    private:
        [[nodiscard]] const InputChannelId* InputChannelFromKeyEvent(xcb_keycode_t code) const;

        static AZStd::string TextFromKeycode(xkb_state* state, xkb_keycode_t code);

        void UpdateState(const xcb_xkb_state_notify_event_t* state);

        XcbUniquePtr<xkb_context, xkb_context_unref> m_xkbContext;
        XcbUniquePtr<xkb_keymap, xkb_keymap_unref> m_xkbKeymap;
        XcbUniquePtr<xkb_state, xkb_state_unref> m_xkbState;
        int m_coreDeviceId{-1};
        uint8_t m_xkbEventCode{0};
        bool m_initialized{false};
        bool m_hasTextEntryStarted{false};
    };
} // namespace AzFramework
