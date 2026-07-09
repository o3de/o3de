/*
* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/WaylandInputDeviceKeyboard.h>
#include <AzFramework/WaylandNativeWindow.h>
#include <sys/mman.h>

namespace AzFramework
{
    wl_keyboard_listener WaylandInputDeviceKeyboard::s_keyboardListener = {
        .keymap = KeyboardKeymap,
        .enter = KeyboardEnter,
        .leave = KeyboardLeave,
        .key = KeyboardKey,
        .modifiers = KeyboardModifiers,
        .repeat_info = KeyboardRepeatInfo
    };

    void WaylandInputDeviceKeyboard::KeyboardEnter(
        void* data, wl_keyboard*, uint32_t serial, wl_surface* surface, wl_array* keys)
    {
        auto self = static_cast<WaylandInputDeviceKeyboard*>(data);
        self->m_focusedWindow = static_cast<NativeWindowHandle>(surface);
        self->m_currentSerial = serial;

        uint32_t* key;
        wl_array_for_each_cpp(key, keys, uint32_t)
        {
            self->SendKeyEvent(*key, true);
        }
    }

    void WaylandInputDeviceKeyboard::KeyboardLeave(void* data, wl_keyboard*, uint32_t /*serial*/,
                                                   wl_surface* surface)
    {
        auto self = static_cast<WaylandInputDeviceKeyboard*>(data);
        self->m_focusedWindow = nullptr;

        self->m_focusedWindow = nullptr;
        self->m_currentSerial = UINT32_MAX;
    }

    void WaylandInputDeviceKeyboard::KeyboardKeymap(void* data, wl_keyboard*, uint32_t format,
                                                    int32_t fd, uint32_t size)
    {
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
        {
            AZ_Error("WaylandInputDeviceKeyboard", false, "Given keyboard format isn't XKB_V1");
            return;
        }
        auto self = static_cast<WaylandInputDeviceKeyboard*>(data);

        char* map_shm = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (map_shm == MAP_FAILED)
        {
            AZ_Error("WaylandInputDeviceKeyboard", map_shm != MAP_FAILED, "Failed to MMAP: %s", strerror(errno));
            return;
        }

        xkb_keymap* xkb_keymap =
            xkb_keymap_new_from_string(self->m_xkbContext, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(map_shm, size);
        close(fd);

        auto xkb_state = xkb_state_new(xkb_keymap);
        xkb_keymap_unref(self->m_xkbKeymap);
        xkb_state_unref(self->m_xkbState);
        self->m_xkbKeymap = xkb_keymap;
        self->m_xkbState = xkb_state;
    }

    void WaylandInputDeviceKeyboard::KeyboardKey(
        void* data, wl_keyboard*, uint32_t /*serial*/, uint32_t /*time*/, uint32_t key, uint32_t state)
    {
        auto self = static_cast<WaylandInputDeviceKeyboard*>(data);
        self->SendKeyEvent(key, state == WL_KEYBOARD_KEY_STATE_PRESSED || state == WL_KEYBOARD_KEY_STATE_REPEATED);
    }

    void WaylandInputDeviceKeyboard::KeyboardModifiers(
        void* data,
        wl_keyboard*,
        uint32_t /*serial*/,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group)
    {
        auto self = static_cast<WaylandInputDeviceKeyboard*>(data);
        xkb_state_update_mask(self->m_xkbState, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    }

    void WaylandInputDeviceKeyboard::KeyboardRepeatInfo(void* data, wl_keyboard*, int32_t rate,
                                                        int32_t delay)
    {
        auto self = static_cast<WaylandInputDeviceKeyboard*>(data);
        self->m_repeatDelayMs = delay;
        self->m_repeatRatePerSec = rate;
    }

    WaylandInputDeviceKeyboard::WaylandInputDeviceKeyboard(AzFramework::InputDeviceKeyboard& inputDevice)
        : InputDeviceKeyboard::Implementation(inputDevice)
          , m_playerIdx(inputDevice.GetInputDeviceId().GetIndex())
    {
        SeatCapsChanged();
        SeatNotificationsBus::Handler::BusConnect(m_playerIdx);

        if (auto wl = WaylandConnectionManagerInterface::Get())
        {
            m_xkbContext = wl->GetXkbContext();
        }
    }

    WaylandInputDeviceKeyboard::~WaylandInputDeviceKeyboard()
    {
        SeatNotificationsBus::Handler::BusDisconnect();
        UpdateKeyboard(nullptr);
    }

    WaylandInputDeviceKeyboard::Implementation* WaylandInputDeviceKeyboard::Create(
        AzFramework::InputDeviceKeyboard& inputDevice)
    {
        return aznew WaylandInputDeviceKeyboard(inputDevice);
    }

    void WaylandInputDeviceKeyboard::UpdateKeyboard(wl_keyboard* newKeyboard)
    {
        if (newKeyboard == m_keyboard)
        {
            return;
        }

        if (m_keyboard != nullptr)
        {
            wl_keyboard_release(m_keyboard);
        }

        m_keyboard = newKeyboard;
        if (m_keyboard)
        {
            wl_keyboard_add_listener(m_keyboard, &s_keyboardListener, this);
        }
    }

    void WaylandInputDeviceKeyboard::ReleaseSeat()
    {
        UpdateKeyboard(nullptr);
    }

    void WaylandInputDeviceKeyboard::SeatCapsChanged()
    {
        const auto* interface = AzFramework::SeatManagerInterface::Get();
        if (!interface)
        {
            return;
        }

        UpdateKeyboard(interface->GetSeatKeyboard(m_playerIdx));
    }

    bool WaylandInputDeviceKeyboard::IsConnected() const
    {
        return m_keyboard != nullptr;
    }

    bool WaylandInputDeviceKeyboard::HasTextEntryStarted() const
    {
        return m_inTextMode;
    }

    void WaylandInputDeviceKeyboard::TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions& options)
    {
        m_inTextMode = true;
    }

    void WaylandInputDeviceKeyboard::TextEntryStop()
    {
        m_inTextMode = false;
    }

    void WaylandInputDeviceKeyboard::ResetRepeatState()
    {
        m_currentHeldKey.clear();
        m_heldKeyElapsed = AZ::TimeMs();
        m_lastRepeatTime = AZ::TimeMs();
    }

    void WaylandInputDeviceKeyboard::TickInputDevice()
    {
        //Need version 10 to get repeated key events.
        //find out if wl_keybaord v10 is supported in the majority of compositors before removing.
        if (!m_currentHeldKey.empty() && m_repeatRatePerSec != 0)
        {
            AZ::TimeMs currentElapsed;
            AZ::ITimeRequestBus::BroadcastResult(currentElapsed, &AZ::ITimeRequestBus::Events::GetRealElapsedTimeMs);

            auto heldTimeMs = currentElapsed - m_heldKeyElapsed;
            if (heldTimeMs >= AZ::TimeMs(m_repeatDelayMs))
            {
                // Delay is done, repeat
                auto repeatIntervalMs = AZ::TimeMs(1000 / m_repeatRatePerSec);
                auto timeSinceLastRepeat = currentElapsed - m_lastRepeatTime;

                if (timeSinceLastRepeat >= repeatIntervalMs)
                {
                    QueueRawTextEvent(m_currentHeldKey);
                    m_lastRepeatTime = currentElapsed;
                }
            }
        }

        ProcessRawEventQueues();
    }

    void WaylandInputDeviceKeyboard::SendKeyEvent(uint32_t waylandKey, bool isPressed)
    {
        xkb_keysym_t sym = xkb_state_key_get_one_sym(m_xkbState, waylandKey + 8);

        auto text = TextFromKeyCode(waylandKey + 8);
        if (isPressed)
        {
            if (!text.empty())
            {
                if (text != m_currentHeldKey)
                {
                    // Not the same key so reset repeat state
                    ResetRepeatState();
                }
                m_currentHeldKey = text;
                AZ::ITimeRequestBus::BroadcastResult(m_heldKeyElapsed,
                                                     &AZ::ITimeRequestBus::Events::GetRealElapsedTimeMs);

                QueueRawTextEvent(AZStd::move(text));
            }
        }
        else if (m_currentHeldKey == text) // The held key is released, and it's the same.
        {
            ResetRepeatState();
        }

        if (const InputChannelId* key = InputChannelFromKeySym(sym))
        {
            QueueRawKeyEvent(*key, isPressed);
        }
    }

    const InputChannelId* WaylandInputDeviceKeyboard::InputChannelFromKeySym(xkb_keysym_t keysym)
    {
        switch (keysym)
        {
        case XKB_KEY_0:
            return &InputDeviceKeyboard::Key::Alphanumeric0;
        case XKB_KEY_1:
            return &InputDeviceKeyboard::Key::Alphanumeric1;
        case XKB_KEY_2:
            return &InputDeviceKeyboard::Key::Alphanumeric2;
        case XKB_KEY_3:
            return &InputDeviceKeyboard::Key::Alphanumeric3;
        case XKB_KEY_4:
            return &InputDeviceKeyboard::Key::Alphanumeric4;
        case XKB_KEY_5:
            return &InputDeviceKeyboard::Key::Alphanumeric5;
        case XKB_KEY_6:
            return &InputDeviceKeyboard::Key::Alphanumeric6;
        case XKB_KEY_7:
            return &InputDeviceKeyboard::Key::Alphanumeric7;
        case XKB_KEY_8:
            return &InputDeviceKeyboard::Key::Alphanumeric8;
        case XKB_KEY_9:
            return &InputDeviceKeyboard::Key::Alphanumeric9;
        case XKB_KEY_A:
        case XKB_KEY_a:
            return &InputDeviceKeyboard::Key::AlphanumericA;
        case XKB_KEY_B:
        case XKB_KEY_b:
            return &InputDeviceKeyboard::Key::AlphanumericB;
        case XKB_KEY_C:
        case XKB_KEY_c:
            return &InputDeviceKeyboard::Key::AlphanumericC;
        case XKB_KEY_D:
        case XKB_KEY_d:
            return &InputDeviceKeyboard::Key::AlphanumericD;
        case XKB_KEY_E:
        case XKB_KEY_e:
            return &InputDeviceKeyboard::Key::AlphanumericE;
        case XKB_KEY_F:
        case XKB_KEY_f:
            return &InputDeviceKeyboard::Key::AlphanumericF;
        case XKB_KEY_G:
        case XKB_KEY_g:
            return &InputDeviceKeyboard::Key::AlphanumericG;
        case XKB_KEY_H:
        case XKB_KEY_h:
            return &InputDeviceKeyboard::Key::AlphanumericH;
        case XKB_KEY_I:
        case XKB_KEY_i:
            return &InputDeviceKeyboard::Key::AlphanumericI;
        case XKB_KEY_J:
        case XKB_KEY_j:
            return &InputDeviceKeyboard::Key::AlphanumericJ;
        case XKB_KEY_K:
        case XKB_KEY_k:
            return &InputDeviceKeyboard::Key::AlphanumericK;
        case XKB_KEY_L:
        case XKB_KEY_l:
            return &InputDeviceKeyboard::Key::AlphanumericL;
        case XKB_KEY_M:
        case XKB_KEY_m:
            return &InputDeviceKeyboard::Key::AlphanumericM;
        case XKB_KEY_N:
        case XKB_KEY_n:
            return &InputDeviceKeyboard::Key::AlphanumericN;
        case XKB_KEY_O:
        case XKB_KEY_o:
            return &InputDeviceKeyboard::Key::AlphanumericO;
        case XKB_KEY_P:
        case XKB_KEY_p:
            return &InputDeviceKeyboard::Key::AlphanumericP;
        case XKB_KEY_Q:
        case XKB_KEY_q:
            return &InputDeviceKeyboard::Key::AlphanumericQ;
        case XKB_KEY_R:
        case XKB_KEY_r:
            return &InputDeviceKeyboard::Key::AlphanumericR;
        case XKB_KEY_S:
        case XKB_KEY_s:
            return &InputDeviceKeyboard::Key::AlphanumericS;
        case XKB_KEY_T:
        case XKB_KEY_t:
            return &InputDeviceKeyboard::Key::AlphanumericT;
        case XKB_KEY_U:
        case XKB_KEY_u:
            return &InputDeviceKeyboard::Key::AlphanumericU;
        case XKB_KEY_V:
        case XKB_KEY_v:
            return &InputDeviceKeyboard::Key::AlphanumericV;
        case XKB_KEY_W:
        case XKB_KEY_w:
            return &InputDeviceKeyboard::Key::AlphanumericW;
        case XKB_KEY_X:
        case XKB_KEY_x:
            return &InputDeviceKeyboard::Key::AlphanumericX;
        case XKB_KEY_Y:
        case XKB_KEY_y:
            return &InputDeviceKeyboard::Key::AlphanumericY;
        case XKB_KEY_Z:
        case XKB_KEY_z:
            return &InputDeviceKeyboard::Key::AlphanumericZ;
        case XKB_KEY_BackSpace:
            return &InputDeviceKeyboard::Key::EditBackspace;
        case XKB_KEY_Caps_Lock:
            return &InputDeviceKeyboard::Key::EditCapsLock;
        case XKB_KEY_Return:
            return &InputDeviceKeyboard::Key::EditEnter;
        case XKB_KEY_space:
            return &InputDeviceKeyboard::Key::EditSpace;
        case XKB_KEY_Tab:
            return &InputDeviceKeyboard::Key::EditTab;
        case XKB_KEY_Escape:
            return &InputDeviceKeyboard::Key::Escape;
        case XKB_KEY_F1:
            return &InputDeviceKeyboard::Key::Function01;
        case XKB_KEY_F2:
            return &InputDeviceKeyboard::Key::Function02;
        case XKB_KEY_F3:
            return &InputDeviceKeyboard::Key::Function03;
        case XKB_KEY_F4:
            return &InputDeviceKeyboard::Key::Function04;
        case XKB_KEY_F5:
            return &InputDeviceKeyboard::Key::Function05;
        case XKB_KEY_F6:
            return &InputDeviceKeyboard::Key::Function06;
        case XKB_KEY_F7:
            return &InputDeviceKeyboard::Key::Function07;
        case XKB_KEY_F8:
            return &InputDeviceKeyboard::Key::Function08;
        case XKB_KEY_F9:
            return &InputDeviceKeyboard::Key::Function09;
        case XKB_KEY_F10:
            return &InputDeviceKeyboard::Key::Function10;
        case XKB_KEY_F11:
            return &InputDeviceKeyboard::Key::Function11;
        case XKB_KEY_F12:
            return &InputDeviceKeyboard::Key::Function12;
        case XKB_KEY_F13:
            return &InputDeviceKeyboard::Key::Function13;
        case XKB_KEY_F14:
            return &InputDeviceKeyboard::Key::Function14;
        case XKB_KEY_F15:
            return &InputDeviceKeyboard::Key::Function15;
        case XKB_KEY_F16:
            return &InputDeviceKeyboard::Key::Function16;
        case XKB_KEY_F17:
            return &InputDeviceKeyboard::Key::Function17;
        case XKB_KEY_F18:
            return &InputDeviceKeyboard::Key::Function18;
        case XKB_KEY_F19:
            return &InputDeviceKeyboard::Key::Function19;
        case XKB_KEY_F20:
            return &InputDeviceKeyboard::Key::Function20;
        case XKB_KEY_Alt_L:
            return &InputDeviceKeyboard::Key::ModifierAltL;
        case XKB_KEY_Alt_R:
            return &InputDeviceKeyboard::Key::ModifierAltR;
        case XKB_KEY_Control_L:
            return &InputDeviceKeyboard::Key::ModifierCtrlL;
        case XKB_KEY_Control_R:
            return &InputDeviceKeyboard::Key::ModifierCtrlR;
        case XKB_KEY_Shift_L:
            return &InputDeviceKeyboard::Key::ModifierShiftL;
        case XKB_KEY_Shift_R:
            return &InputDeviceKeyboard::Key::ModifierShiftR;
        case XKB_KEY_Super_L:
            return &InputDeviceKeyboard::Key::ModifierSuperL;
        case XKB_KEY_Super_R:
            return &InputDeviceKeyboard::Key::ModifierSuperR;
        case XKB_KEY_Down:
            return &InputDeviceKeyboard::Key::NavigationArrowDown;
        case XKB_KEY_Left:
            return &InputDeviceKeyboard::Key::NavigationArrowLeft;
        case XKB_KEY_Right:
            return &InputDeviceKeyboard::Key::NavigationArrowRight;
        case XKB_KEY_Up:
            return &InputDeviceKeyboard::Key::NavigationArrowUp;
        case XKB_KEY_Delete:
            return &InputDeviceKeyboard::Key::NavigationDelete;
        case XKB_KEY_End:
            return &InputDeviceKeyboard::Key::NavigationEnd;
        case XKB_KEY_Home:
            return &InputDeviceKeyboard::Key::NavigationHome;
        case XKB_KEY_Insert:
            return &InputDeviceKeyboard::Key::NavigationInsert;
        case XKB_KEY_Page_Down:
            return &InputDeviceKeyboard::Key::NavigationPageDown;
        case XKB_KEY_Page_Up:
            return &InputDeviceKeyboard::Key::NavigationPageUp;
        case XKB_KEY_Num_Lock:
            return &InputDeviceKeyboard::Key::NumLock;
        case XKB_KEY_KP_0:
            return &InputDeviceKeyboard::Key::NumPad0;
        case XKB_KEY_KP_1:
            return &InputDeviceKeyboard::Key::NumPad1;
        case XKB_KEY_KP_2:
            return &InputDeviceKeyboard::Key::NumPad2;
        case XKB_KEY_KP_3:
            return &InputDeviceKeyboard::Key::NumPad3;
        case XKB_KEY_KP_4:
            return &InputDeviceKeyboard::Key::NumPad4;
        case XKB_KEY_KP_5:
            return &InputDeviceKeyboard::Key::NumPad5;
        case XKB_KEY_KP_6:
            return &InputDeviceKeyboard::Key::NumPad6;
        case XKB_KEY_KP_7:
            return &InputDeviceKeyboard::Key::NumPad7;
        case XKB_KEY_KP_8:
            return &InputDeviceKeyboard::Key::NumPad8;
        case XKB_KEY_KP_9:
            return &InputDeviceKeyboard::Key::NumPad9;
        case XKB_KEY_KP_Add:
            return &InputDeviceKeyboard::Key::NumPadAdd;
        case XKB_KEY_KP_Decimal:
            return &InputDeviceKeyboard::Key::NumPadDecimal;
        case XKB_KEY_KP_Divide:
            return &InputDeviceKeyboard::Key::NumPadDivide;
        case XKB_KEY_KP_Enter:
            return &InputDeviceKeyboard::Key::NumPadEnter;
        case XKB_KEY_KP_Multiply:
            return &InputDeviceKeyboard::Key::NumPadMultiply;
        case XKB_KEY_KP_Subtract:
            return &InputDeviceKeyboard::Key::NumPadSubtract;
        case XKB_KEY_apostrophe:
            return &InputDeviceKeyboard::Key::PunctuationApostrophe;
        case XKB_KEY_backslash:
            return &InputDeviceKeyboard::Key::PunctuationBackslash;
        case XKB_KEY_bracketleft:
            return &InputDeviceKeyboard::Key::PunctuationBracketL;
        case XKB_KEY_bracketright:
            return &InputDeviceKeyboard::Key::PunctuationBracketR;
        case XKB_KEY_comma:
            return &InputDeviceKeyboard::Key::PunctuationComma;
        case XKB_KEY_equal:
            return &InputDeviceKeyboard::Key::PunctuationEquals;
        case XKB_KEY_hyphen:
            return &InputDeviceKeyboard::Key::PunctuationHyphen;
        case XKB_KEY_period:
            return &InputDeviceKeyboard::Key::PunctuationPeriod;
        case XKB_KEY_semicolon:
            return &InputDeviceKeyboard::Key::PunctuationSemicolon;
        case XKB_KEY_slash:
            return &InputDeviceKeyboard::Key::PunctuationSlash;
        case XKB_KEY_grave:
        case XKB_KEY_asciitilde:
            return &InputDeviceKeyboard::Key::PunctuationTilde;
        case XKB_KEY_ISO_Group_Shift:
            return &InputDeviceKeyboard::Key::SupplementaryISO;
        case XKB_KEY_Pause:
            return &InputDeviceKeyboard::Key::WindowsSystemPause;
        case XKB_KEY_Print:
            return &InputDeviceKeyboard::Key::WindowsSystemPrint;
        case XKB_KEY_Scroll_Lock:
            return &InputDeviceKeyboard::Key::WindowsSystemScrollLock;
        default:
            return nullptr;
        }
    }

    AZStd::string WaylandInputDeviceKeyboard::TextFromKeyCode(xkb_keycode_t code)
    {
        const size_t size = xkb_state_key_get_utf8(m_xkbState, code, nullptr, 0);
        if (!size)
        {
            return {};
        }

        AZStd::string chars;
        chars.resize_no_construct(size);
        xkb_state_key_get_utf8(m_xkbState, code, chars.data(), chars.size() + 1);
        return chars;
    }
} // namespace AzFramework
