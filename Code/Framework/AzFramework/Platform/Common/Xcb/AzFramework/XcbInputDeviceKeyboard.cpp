/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/XcbEventHandler.h>
#include <AzFramework/XcbConnectionManager.h>
#include <AzFramework/XcbInputDeviceKeyboard.h>

#define explicit ExplicitIsACXXKeyword
#include <xcb/xkb.h>
#undef explicit
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>

namespace AzFramework
{
    XcbInputDeviceKeyboard::XcbInputDeviceKeyboard(InputDeviceKeyboard& inputDevice)
        : InputDeviceKeyboard::Implementation(inputDevice)
    {
        XcbEventHandlerBus::Handler::BusConnect();

        auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
        if (!interface)
        {
            AZ_Warning("ApplicationLinux", false, "XCB interface not available");
            return;
        }

        auto* connection = interface->GetXcbConnection();
        if (!connection)
        {
            AZ_Warning("ApplicationLinux", false, "XCB connection not available");
            return;
        }

        XcbStdFreePtr<xcb_xkb_use_extension_reply_t> xkbUseExtensionReply{
            xcb_xkb_use_extension_reply(connection, xcb_xkb_use_extension(connection, 1, 0), nullptr)
        };
        if (!xkbUseExtensionReply)
        {
            AZ_Warning("ApplicationLinux", false, "Failed to initialize the xkb extension");
            return;
        }
        if (!xkbUseExtensionReply->supported)
        {
            AZ_Warning("ApplicationLinux", false, "The X server does not support the xkb extension");
            return;
        }

        m_coreDeviceId = xkb_x11_get_core_keyboard_device_id(connection);

        m_xkbContext.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
        m_xkbKeymap.reset(xkb_x11_keymap_new_from_device(m_xkbContext.get(), connection, m_coreDeviceId, XKB_KEYMAP_COMPILE_NO_FLAGS));
        m_xkbState.reset(xkb_x11_state_new_from_device(m_xkbKeymap.get(), connection, m_coreDeviceId));

        m_initialized = true;
    }

    bool XcbInputDeviceKeyboard::IsConnected() const
    {
        auto* connection = AzFramework::XcbConnectionManagerInterface::Get()->GetXcbConnection();
        return connection && !xcb_connection_has_error(connection);
    }

    bool XcbInputDeviceKeyboard::HasTextEntryStarted() const
    {
        return false;
    }

    void XcbInputDeviceKeyboard::TextEntryStart(const InputDeviceKeyboard::VirtualKeyboardOptions& options)
    {
    }

    void XcbInputDeviceKeyboard::TextEntryStop()
    {
    }

    void XcbInputDeviceKeyboard::TickInputDevice()
    {
        ProcessRawEventQueues();
    }

    void XcbInputDeviceKeyboard::HandleXcbEvent(xcb_generic_event_t* event)
    {
        if (!m_initialized)
        {
            return;
        }

        switch (event->response_type & ~0x80)
        {
        case XCB_KEY_PRESS:
        {
            auto* keyPress = reinterpret_cast<xcb_key_press_event_t*>(event);

            const InputChannelId* key = InputChannelFromKeyEvent(keyPress->detail);
            if (key)
            {
                QueueRawKeyEvent(*key, true);
            }
            break;
        }
        case XCB_KEY_RELEASE:
        {
            auto* keyRelease = reinterpret_cast<xcb_key_release_event_t*>(event);

            const InputChannelId* key = InputChannelFromKeyEvent(keyRelease->detail);
            if (key)
            {
                QueueRawKeyEvent(*key, false);
            }
            break;
        }
        }
    }

    [[nodiscard]] const InputChannelId* XcbInputDeviceKeyboard::InputChannelFromKeyEvent(xcb_keycode_t code) const
    {
        const xcb_keysym_t keysym = xkb_state_key_get_one_sym(m_xkbState.get(), code);

        switch(keysym)
        {
            case XKB_KEY_0: return &InputDeviceKeyboard::Key::Alphanumeric0;
            case XKB_KEY_1: return &InputDeviceKeyboard::Key::Alphanumeric1;
            case XKB_KEY_2: return &InputDeviceKeyboard::Key::Alphanumeric2;
            case XKB_KEY_3: return &InputDeviceKeyboard::Key::Alphanumeric3;
            case XKB_KEY_4: return &InputDeviceKeyboard::Key::Alphanumeric4;
            case XKB_KEY_5: return &InputDeviceKeyboard::Key::Alphanumeric5;
            case XKB_KEY_6: return &InputDeviceKeyboard::Key::Alphanumeric6;
            case XKB_KEY_7: return &InputDeviceKeyboard::Key::Alphanumeric7;
            case XKB_KEY_8: return &InputDeviceKeyboard::Key::Alphanumeric8;
            case XKB_KEY_9: return &InputDeviceKeyboard::Key::Alphanumeric9;
            case XKB_KEY_A:
            case XKB_KEY_a: return &InputDeviceKeyboard::Key::AlphanumericA;
            case XKB_KEY_B:
            case XKB_KEY_b: return &InputDeviceKeyboard::Key::AlphanumericB;
            case XKB_KEY_C:
            case XKB_KEY_c: return &InputDeviceKeyboard::Key::AlphanumericC;
            case XKB_KEY_D:
            case XKB_KEY_d: return &InputDeviceKeyboard::Key::AlphanumericD;
            case XKB_KEY_E:
            case XKB_KEY_e: return &InputDeviceKeyboard::Key::AlphanumericE;
            case XKB_KEY_F:
            case XKB_KEY_f: return &InputDeviceKeyboard::Key::AlphanumericF;
            case XKB_KEY_G:
            case XKB_KEY_g: return &InputDeviceKeyboard::Key::AlphanumericG;
            case XKB_KEY_H:
            case XKB_KEY_h: return &InputDeviceKeyboard::Key::AlphanumericH;
            case XKB_KEY_I:
            case XKB_KEY_i: return &InputDeviceKeyboard::Key::AlphanumericI;
            case XKB_KEY_J:
            case XKB_KEY_j: return &InputDeviceKeyboard::Key::AlphanumericJ;
            case XKB_KEY_K:
            case XKB_KEY_k: return &InputDeviceKeyboard::Key::AlphanumericK;
            case XKB_KEY_L:
            case XKB_KEY_l: return &InputDeviceKeyboard::Key::AlphanumericL;
            case XKB_KEY_M:
            case XKB_KEY_m: return &InputDeviceKeyboard::Key::AlphanumericM;
            case XKB_KEY_N:
            case XKB_KEY_n: return &InputDeviceKeyboard::Key::AlphanumericN;
            case XKB_KEY_O:
            case XKB_KEY_o: return &InputDeviceKeyboard::Key::AlphanumericO;
            case XKB_KEY_P:
            case XKB_KEY_p: return &InputDeviceKeyboard::Key::AlphanumericP;
            case XKB_KEY_Q:
            case XKB_KEY_q: return &InputDeviceKeyboard::Key::AlphanumericQ;
            case XKB_KEY_R:
            case XKB_KEY_r: return &InputDeviceKeyboard::Key::AlphanumericR;
            case XKB_KEY_S:
            case XKB_KEY_s: return &InputDeviceKeyboard::Key::AlphanumericS;
            case XKB_KEY_T:
            case XKB_KEY_t: return &InputDeviceKeyboard::Key::AlphanumericT;
            case XKB_KEY_U:
            case XKB_KEY_u: return &InputDeviceKeyboard::Key::AlphanumericU;
            case XKB_KEY_V:
            case XKB_KEY_v: return &InputDeviceKeyboard::Key::AlphanumericV;
            case XKB_KEY_W:
            case XKB_KEY_w: return &InputDeviceKeyboard::Key::AlphanumericW;
            case XKB_KEY_X:
            case XKB_KEY_x: return &InputDeviceKeyboard::Key::AlphanumericX;
            case XKB_KEY_Y:
            case XKB_KEY_y: return &InputDeviceKeyboard::Key::AlphanumericY;
            case XKB_KEY_Z:
            case XKB_KEY_z: return &InputDeviceKeyboard::Key::AlphanumericZ;
            case XKB_KEY_BackSpace: return &InputDeviceKeyboard::Key::EditBackspace;
            case XKB_KEY_Caps_Lock: return &InputDeviceKeyboard::Key::EditCapsLock;
            case XKB_KEY_Return: return &InputDeviceKeyboard::Key::EditEnter;
            case XKB_KEY_space: return &InputDeviceKeyboard::Key::EditSpace;
            case XKB_KEY_Tab: return &InputDeviceKeyboard::Key::EditTab;
            case XKB_KEY_Escape: return &InputDeviceKeyboard::Key::Escape;
            case XKB_KEY_F1: return &InputDeviceKeyboard::Key::Function01;
            case XKB_KEY_F2: return &InputDeviceKeyboard::Key::Function02;
            case XKB_KEY_F3: return &InputDeviceKeyboard::Key::Function03;
            case XKB_KEY_F4: return &InputDeviceKeyboard::Key::Function04;
            case XKB_KEY_F5: return &InputDeviceKeyboard::Key::Function05;
            case XKB_KEY_F6: return &InputDeviceKeyboard::Key::Function06;
            case XKB_KEY_F7: return &InputDeviceKeyboard::Key::Function07;
            case XKB_KEY_F8: return &InputDeviceKeyboard::Key::Function08;
            case XKB_KEY_F9: return &InputDeviceKeyboard::Key::Function09;
            case XKB_KEY_F10: return &InputDeviceKeyboard::Key::Function10;
            case XKB_KEY_F11: return &InputDeviceKeyboard::Key::Function11;
            case XKB_KEY_F12: return &InputDeviceKeyboard::Key::Function12;
            case XKB_KEY_F13: return &InputDeviceKeyboard::Key::Function13;
            case XKB_KEY_F14: return &InputDeviceKeyboard::Key::Function14;
            case XKB_KEY_F15: return &InputDeviceKeyboard::Key::Function15;
            case XKB_KEY_F16: return &InputDeviceKeyboard::Key::Function16;
            case XKB_KEY_F17: return &InputDeviceKeyboard::Key::Function17;
            case XKB_KEY_F18: return &InputDeviceKeyboard::Key::Function18;
            case XKB_KEY_F19: return &InputDeviceKeyboard::Key::Function19;
            case XKB_KEY_F20: return &InputDeviceKeyboard::Key::Function20;
            case XKB_KEY_Alt_L: return &InputDeviceKeyboard::Key::ModifierAltL;
            case XKB_KEY_Alt_R: return &InputDeviceKeyboard::Key::ModifierAltR;
            case XKB_KEY_Control_L: return &InputDeviceKeyboard::Key::ModifierCtrlL;
            case XKB_KEY_Control_R: return &InputDeviceKeyboard::Key::ModifierCtrlR;
            case XKB_KEY_Shift_L: return &InputDeviceKeyboard::Key::ModifierShiftL;
            case XKB_KEY_Shift_R: return &InputDeviceKeyboard::Key::ModifierShiftR;
            case XKB_KEY_Super_L: return &InputDeviceKeyboard::Key::ModifierSuperL;
            case XKB_KEY_Super_R: return &InputDeviceKeyboard::Key::ModifierSuperR;
            case XKB_KEY_Down: return &InputDeviceKeyboard::Key::NavigationArrowDown;
            case XKB_KEY_Left: return &InputDeviceKeyboard::Key::NavigationArrowLeft;
            case XKB_KEY_Right: return &InputDeviceKeyboard::Key::NavigationArrowRight;
            case XKB_KEY_Up: return &InputDeviceKeyboard::Key::NavigationArrowUp;
            case XKB_KEY_Delete: return &InputDeviceKeyboard::Key::NavigationDelete;
            case XKB_KEY_End: return &InputDeviceKeyboard::Key::NavigationEnd;
            case XKB_KEY_Home: return &InputDeviceKeyboard::Key::NavigationHome;
            case XKB_KEY_Insert: return &InputDeviceKeyboard::Key::NavigationInsert;
            case XKB_KEY_Page_Down: return &InputDeviceKeyboard::Key::NavigationPageDown;
            case XKB_KEY_Page_Up: return &InputDeviceKeyboard::Key::NavigationPageUp;
            case XKB_KEY_Num_Lock: return &InputDeviceKeyboard::Key::NumLock;
            case XKB_KEY_KP_0: return &InputDeviceKeyboard::Key::NumPad0;
            case XKB_KEY_KP_1: return &InputDeviceKeyboard::Key::NumPad1;
            case XKB_KEY_KP_2: return &InputDeviceKeyboard::Key::NumPad2;
            case XKB_KEY_KP_3: return &InputDeviceKeyboard::Key::NumPad3;
            case XKB_KEY_KP_4: return &InputDeviceKeyboard::Key::NumPad4;
            case XKB_KEY_KP_5: return &InputDeviceKeyboard::Key::NumPad5;
            case XKB_KEY_KP_6: return &InputDeviceKeyboard::Key::NumPad6;
            case XKB_KEY_KP_7: return &InputDeviceKeyboard::Key::NumPad7;
            case XKB_KEY_KP_8: return &InputDeviceKeyboard::Key::NumPad8;
            case XKB_KEY_KP_9: return &InputDeviceKeyboard::Key::NumPad9;
            case XKB_KEY_KP_Add: return &InputDeviceKeyboard::Key::NumPadAdd;
            case XKB_KEY_KP_Decimal: return &InputDeviceKeyboard::Key::NumPadDecimal;
            case XKB_KEY_KP_Divide: return &InputDeviceKeyboard::Key::NumPadDivide;
            case XKB_KEY_KP_Enter: return &InputDeviceKeyboard::Key::NumPadEnter;
            case XKB_KEY_KP_Multiply: return &InputDeviceKeyboard::Key::NumPadMultiply;
            case XKB_KEY_KP_Subtract: return &InputDeviceKeyboard::Key::NumPadSubtract;
            case XKB_KEY_apostrophe: return &InputDeviceKeyboard::Key::PunctuationApostrophe;
            case XKB_KEY_backslash: return &InputDeviceKeyboard::Key::PunctuationBackslash;
            case XKB_KEY_bracketleft: return &InputDeviceKeyboard::Key::PunctuationBracketL;
            case XKB_KEY_bracketright: return &InputDeviceKeyboard::Key::PunctuationBracketR;
            case XKB_KEY_comma: return &InputDeviceKeyboard::Key::PunctuationComma;
            case XKB_KEY_equal: return &InputDeviceKeyboard::Key::PunctuationEquals;
            case XKB_KEY_hyphen: return &InputDeviceKeyboard::Key::PunctuationHyphen;
            case XKB_KEY_period: return &InputDeviceKeyboard::Key::PunctuationPeriod;
            case XKB_KEY_semicolon: return &InputDeviceKeyboard::Key::PunctuationSemicolon;
            case XKB_KEY_slash: return &InputDeviceKeyboard::Key::PunctuationSlash;
            case XKB_KEY_grave:
            case XKB_KEY_asciitilde: return &InputDeviceKeyboard::Key::PunctuationTilde;
            case XKB_KEY_ISO_Group_Shift: return &InputDeviceKeyboard::Key::SupplementaryISO;
            case XKB_KEY_Pause: return &InputDeviceKeyboard::Key::WindowsSystemPause;
            case XKB_KEY_Print: return &InputDeviceKeyboard::Key::WindowsSystemPrint;
            case XKB_KEY_Scroll_Lock: return &InputDeviceKeyboard::Key::WindowsSystemScrollLock;
            default: return nullptr;
        }
    }
} // namespace AzFramework
