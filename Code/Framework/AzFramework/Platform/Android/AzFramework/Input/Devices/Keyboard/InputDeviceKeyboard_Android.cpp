/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

#include <AzCore/Android/Utils.h>
#include <android/input.h>


namespace
{
    using namespace AzFramework;

    // Table of key ids indexed by their android key code
    const AZStd::array<const InputChannelId*, 285> InputChannelIdByKeyCodeTable =
    {{
        nullptr,                                                //   0   AKEYCODE_UNKNOWN
        nullptr,                                                //   1   AKEYCODE_SOFT_LEFT
        nullptr,                                                //   2   AKEYCODE_SOFT_RIGHT
        nullptr,                                                //   3   AKEYCODE_HOME
        nullptr,                                                //   4   AKEYCODE_BACK
        nullptr,                                                //   5   AKEYCODE_CALL
        nullptr,                                                //   6   AKEYCODE_ENDCALL
        &InputDeviceKeyboard::Key::Alphanumeric0,               //   7   AKEYCODE_0
        &InputDeviceKeyboard::Key::Alphanumeric1,               //   8   AKEYCODE_1
        &InputDeviceKeyboard::Key::Alphanumeric2,               //   9   AKEYCODE_2
        &InputDeviceKeyboard::Key::Alphanumeric3,               //  10   AKEYCODE_3
        &InputDeviceKeyboard::Key::Alphanumeric4,               //  11   AKEYCODE_4
        &InputDeviceKeyboard::Key::Alphanumeric5,               //  12   AKEYCODE_5
        &InputDeviceKeyboard::Key::Alphanumeric6,               //  13   AKEYCODE_6
        &InputDeviceKeyboard::Key::Alphanumeric7,               //  14   AKEYCODE_7
        &InputDeviceKeyboard::Key::Alphanumeric8,               //  15   AKEYCODE_8
        &InputDeviceKeyboard::Key::Alphanumeric9,               //  16   AKEYCODE_9
        nullptr,                                                //  17   AKEYCODE_STAR
        nullptr,                                                //  18   AKEYCODE_POUND
        &InputDeviceKeyboard::Key::NavigationArrowUp,           //  19   AKEYCODE_DPAD_UP
        &InputDeviceKeyboard::Key::NavigationArrowDown,         //  20   AKEYCODE_DPAD_DOWN
        &InputDeviceKeyboard::Key::NavigationArrowLeft,         //  21   AKEYCODE_DPAD_LEFT
        &InputDeviceKeyboard::Key::NavigationArrowRight,        //  22   AKEYCODE_DPAD_RIGHT
        nullptr,                                                //  23   AKEYCODE_DPAD_CENTER
        nullptr,                                                //  24   AKEYCODE_VOLUME_UP
        nullptr,                                                //  25   AKEYCODE_VOLUME_DOWN
        nullptr,                                                //  26   AKEYCODE_POWER
        nullptr,                                                //  27   AKEYCODE_CAMERA
        nullptr,                                                //  28   AKEYCODE_CLEAR
        &InputDeviceKeyboard::Key::AlphanumericA,               //  29   AKEYCODE_A
        &InputDeviceKeyboard::Key::AlphanumericB,               //  30   AKEYCODE_B
        &InputDeviceKeyboard::Key::AlphanumericC,               //  31   AKEYCODE_C
        &InputDeviceKeyboard::Key::AlphanumericD,               //  32   AKEYCODE_D
        &InputDeviceKeyboard::Key::AlphanumericE,               //  33   AKEYCODE_E
        &InputDeviceKeyboard::Key::AlphanumericF,               //  34   AKEYCODE_F
        &InputDeviceKeyboard::Key::AlphanumericG,               //  35   AKEYCODE_G
        &InputDeviceKeyboard::Key::AlphanumericH,               //  36   AKEYCODE_H
        &InputDeviceKeyboard::Key::AlphanumericI,               //  37   AKEYCODE_I
        &InputDeviceKeyboard::Key::AlphanumericJ,               //  38   AKEYCODE_J
        &InputDeviceKeyboard::Key::AlphanumericK,               //  39   AKEYCODE_K
        &InputDeviceKeyboard::Key::AlphanumericL,               //  40   AKEYCODE_L
        &InputDeviceKeyboard::Key::AlphanumericM,               //  41   AKEYCODE_M
        &InputDeviceKeyboard::Key::AlphanumericN,               //  42   AKEYCODE_N
        &InputDeviceKeyboard::Key::AlphanumericO,               //  43   AKEYCODE_O
        &InputDeviceKeyboard::Key::AlphanumericP,               //  44   AKEYCODE_P
        &InputDeviceKeyboard::Key::AlphanumericQ,               //  45   AKEYCODE_Q
        &InputDeviceKeyboard::Key::AlphanumericR,               //  46   AKEYCODE_R
        &InputDeviceKeyboard::Key::AlphanumericS,               //  47   AKEYCODE_S
        &InputDeviceKeyboard::Key::AlphanumericT,               //  48   AKEYCODE_T
        &InputDeviceKeyboard::Key::AlphanumericU,               //  49   AKEYCODE_U
        &InputDeviceKeyboard::Key::AlphanumericV,               //  50   AKEYCODE_V
        &InputDeviceKeyboard::Key::AlphanumericW,               //  51   AKEYCODE_W
        &InputDeviceKeyboard::Key::AlphanumericX,               //  52   AKEYCODE_X
        &InputDeviceKeyboard::Key::AlphanumericY,               //  53   AKEYCODE_Y
        &InputDeviceKeyboard::Key::AlphanumericZ,               //  54   AKEYCODE_Z
        &InputDeviceKeyboard::Key::PunctuationComma,            //  55   AKEYCODE_COMMA
        &InputDeviceKeyboard::Key::PunctuationPeriod,           //  56   AKEYCODE_PERIOD
        &InputDeviceKeyboard::Key::ModifierAltL,                //  57   AKEYCODE_ALT_LEFT
        &InputDeviceKeyboard::Key::ModifierAltR,                //  58   AKEYCODE_ALT_RIGHT
        &InputDeviceKeyboard::Key::ModifierShiftL,              //  59   AKEYCODE_SHIFT_LEFT
        &InputDeviceKeyboard::Key::ModifierShiftR,              //  60   AKEYCODE_SHIFT_RIGHT
        &InputDeviceKeyboard::Key::EditTab,                     //  61   AKEYCODE_TAB
        &InputDeviceKeyboard::Key::EditSpace,                   //  62   AKEYCODE_SPACE
        nullptr,                                                //  63   AKEYCODE_SYM
        nullptr,                                                //  64   AKEYCODE_EXPLORER
        nullptr,                                                //  65   AKEYCODE_ENVELOPE
        &InputDeviceKeyboard::Key::EditEnter,                   //  66   AKEYCODE_ENTER
        &InputDeviceKeyboard::Key::EditBackspace,               //  67   AKEYCODE_DEL
        &InputDeviceKeyboard::Key::PunctuationTilde,            //  68   AKEYCODE_GRAVE
        &InputDeviceKeyboard::Key::PunctuationHyphen,           //  69   AKEYCODE_MINUS
        &InputDeviceKeyboard::Key::PunctuationEquals,           //  70   AKEYCODE_EQUALS
        &InputDeviceKeyboard::Key::PunctuationBracketL,         //  71   AKEYCODE_LEFT_BRACKET
        &InputDeviceKeyboard::Key::PunctuationBracketR,         //  72   AKEYCODE_RIGHT_BRACKET
        &InputDeviceKeyboard::Key::PunctuationBackslash,        //  73   AKEYCODE_BACKSLASH
        &InputDeviceKeyboard::Key::PunctuationSemicolon,        //  74   AKEYCODE_SEMICOLON
        &InputDeviceKeyboard::Key::PunctuationApostrophe,       //  75   AKEYCODE_APOSTROPHE
        &InputDeviceKeyboard::Key::PunctuationSlash,            //  76   AKEYCODE_SLASH
        nullptr,                                                //  77   AKEYCODE_AT
        nullptr,                                                //  78   AKEYCODE_NUM
        nullptr,                                                //  79   AKEYCODE_HEADSETHOOK
        nullptr,                                                //  80   AKEYCODE_FOCUS
        nullptr,                                                //  81   AKEYCODE_PLUS
        nullptr,                                                //  82   AKEYCODE_MENU
        nullptr,                                                //  83   AKEYCODE_NOTIFICATION
        nullptr,                                                //  84   AKEYCODE_SEARCH
        nullptr,                                                //  85   AKEYCODE_MEDIA_PLAY_PAUSE
        nullptr,                                                //  86   AKEYCODE_MEDIA_STOP
        nullptr,                                                //  87   AKEYCODE_MEDIA_NEXT
        nullptr,                                                //  88   AKEYCODE_MEDIA_PREVIOUS
        nullptr,                                                //  89   AKEYCODE_MEDIA_REWIND
        nullptr,                                                //  90   AKEYCODE_MEDIA_FAST_FORWARD
        nullptr,                                                //  91   AKEYCODE_MUTE
        &InputDeviceKeyboard::Key::NavigationPageUp,            //  92   AKEYCODE_PAGE_UP
        &InputDeviceKeyboard::Key::NavigationPageDown,          //  93   AKEYCODE_PAGE_DOWN
        nullptr,                                                //  94   AKEYCODE_PICTSYMBOLS
        nullptr,                                                //  95   AKEYCODE_SWITCH_CHARSET
        nullptr,                                                //  96   AKEYCODE_BUTTON_A
        nullptr,                                                //  97   AKEYCODE_BUTTON_B
        nullptr,                                                //  98   AKEYCODE_BUTTON_C
        nullptr,                                                //  99   AKEYCODE_BUTTON_X
        nullptr,                                                // 100   AKEYCODE_BUTTON_Y
        nullptr,                                                // 101   AKEYCODE_BUTTON_Z
        nullptr,                                                // 102   AKEYCODE_BUTTON_L1
        nullptr,                                                // 103   AKEYCODE_BUTTON_R1
        nullptr,                                                // 104   AKEYCODE_BUTTON_L2
        nullptr,                                                // 105   AKEYCODE_BUTTON_R2
        nullptr,                                                // 106   AKEYCODE_BUTTON_THUMBL
        nullptr,                                                // 107   AKEYCODE_BUTTON_THUMBR
        nullptr,                                                // 108   AKEYCODE_BUTTON_START
        nullptr,                                                // 109   AKEYCODE_BUTTON_SELECT
        nullptr,                                                // 110   AKEYCODE_BUTTON_MODE
        &InputDeviceKeyboard::Key::Escape,                      // 111   AKEYCODE_ESCAPE
        &InputDeviceKeyboard::Key::NavigationDelete,            // 112   AKEYCODE_FORWARD_DEL
        &InputDeviceKeyboard::Key::ModifierCtrlL,               // 113   AKEYCODE_CTRL_LEFT
        &InputDeviceKeyboard::Key::ModifierCtrlR,               // 114   AKEYCODE_CTRL_RIGHT
        &InputDeviceKeyboard::Key::EditCapsLock,                // 115   AKEYCODE_CAPS_LOCK
        &InputDeviceKeyboard::Key::WindowsSystemScrollLock,     // 116   AKEYCODE_SCROLL_LOCK
        nullptr,                                                // 117   AKEYCODE_META_LEFT
        nullptr,                                                // 118   AKEYCODE_META_RIGHT
        nullptr,                                                // 119   AKEYCODE_FUNCTION
        nullptr,                                                // 120   AKEYCODE_SYSRQ
        nullptr,                                                // 121   AKEYCODE_BREAK
        &InputDeviceKeyboard::Key::NavigationHome,              // 122   AKEYCODE_MOVE_HOME
        &InputDeviceKeyboard::Key::NavigationEnd,               // 123   AKEYCODE_MOVE_END
        &InputDeviceKeyboard::Key::NavigationInsert,            // 124   AKEYCODE_INSERT
        nullptr,                                                // 125   AKEYCODE_FORWARD
        nullptr,                                                // 126   AKEYCODE_MEDIA_PLAY
        nullptr,                                                // 127   AKEYCODE_MEDIA_PAUSE
        nullptr,                                                // 128   AKEYCODE_MEDIA_CLOSE
        nullptr,                                                // 129   AKEYCODE_MEDIA_EJECT
        nullptr,                                                // 130   AKEYCODE_MEDIA_RECORD
        &InputDeviceKeyboard::Key::Function01,                  // 131   AKEYCODE_F1
        &InputDeviceKeyboard::Key::Function02,                  // 132   AKEYCODE_F2
        &InputDeviceKeyboard::Key::Function03,                  // 133   AKEYCODE_F3
        &InputDeviceKeyboard::Key::Function04,                  // 134   AKEYCODE_F4
        &InputDeviceKeyboard::Key::Function05,                  // 135   AKEYCODE_F5
        &InputDeviceKeyboard::Key::Function06,                  // 136   AKEYCODE_F6
        &InputDeviceKeyboard::Key::Function07,                  // 137   AKEYCODE_F7
        &InputDeviceKeyboard::Key::Function08,                  // 138   AKEYCODE_F8
        &InputDeviceKeyboard::Key::Function09,                  // 139   AKEYCODE_F9
        &InputDeviceKeyboard::Key::Function10,                  // 140   AKEYCODE_F10
        &InputDeviceKeyboard::Key::Function11,                  // 141   AKEYCODE_F11
        &InputDeviceKeyboard::Key::Function12,                  // 142   AKEYCODE_F12
        &InputDeviceKeyboard::Key::NumLock,                     // 143   AKEYCODE_NUM_LOCK
        &InputDeviceKeyboard::Key::NumPad0,                     // 144   AKEYCODE_NUMPAD_0
        &InputDeviceKeyboard::Key::NumPad1,                     // 145   AKEYCODE_NUMPAD_1
        &InputDeviceKeyboard::Key::NumPad2,                     // 146   AKEYCODE_NUMPAD_2
        &InputDeviceKeyboard::Key::NumPad3,                     // 147   AKEYCODE_NUMPAD_3
        &InputDeviceKeyboard::Key::NumPad4,                     // 148   AKEYCODE_NUMPAD_4
        &InputDeviceKeyboard::Key::NumPad5,                     // 149   AKEYCODE_NUMPAD_5
        &InputDeviceKeyboard::Key::NumPad6,                     // 150   AKEYCODE_NUMPAD_6
        &InputDeviceKeyboard::Key::NumPad7,                     // 151   AKEYCODE_NUMPAD_7
        &InputDeviceKeyboard::Key::NumPad8,                     // 152   AKEYCODE_NUMPAD_8
        &InputDeviceKeyboard::Key::NumPad9,                     // 153   AKEYCODE_NUMPAD_9
        &InputDeviceKeyboard::Key::NumPadDivide,                // 154   AKEYCODE_NUMPAD_DIVIDE
        &InputDeviceKeyboard::Key::NumPadMultiply,              // 155   AKEYCODE_NUMPAD_MULTIPLY
        &InputDeviceKeyboard::Key::NumPadSubtract,              // 156   AKEYCODE_NUMPAD_SUBTRACT
        &InputDeviceKeyboard::Key::NumPadAdd,                   // 157   AKEYCODE_NUMPAD_ADD
        &InputDeviceKeyboard::Key::NumPadDecimal,               // 158   AKEYCODE_NUMPAD_DOT
        nullptr,                                                // 159   AKEYCODE_NUMPAD_COMMA
        &InputDeviceKeyboard::Key::NumPadEnter,                 // 160   AKEYCODE_NUMPAD_ENTER
        nullptr,                                                // 161   AKEYCODE_NUMPAD_EQUALS
        nullptr,                                                // 162   AKEYCODE_NUMPAD_LEFT_PAREN
        nullptr,                                                // 163   AKEYCODE_NUMPAD_RIGHT_PAREN
        nullptr,                                                // 164   AKEYCODE_VOLUME_MUTE
        nullptr,                                                // 165   AKEYCODE_INFO
        nullptr,                                                // 166   AKEYCODE_CHANNEL_UP
        nullptr,                                                // 167   AKEYCODE_CHANNEL_DOWN
        nullptr,                                                // 168   AKEYCODE_ZOOM_IN
        nullptr,                                                // 169   AKEYCODE_ZOOM_OUT
        nullptr,                                                // 170   AKEYCODE_TV
        nullptr,                                                // 171   AKEYCODE_WINDOW
        nullptr,                                                // 172   AKEYCODE_GUIDE
        nullptr,                                                // 173   AKEYCODE_DVR
        nullptr,                                                // 174   AKEYCODE_BOOKMARK
        nullptr,                                                // 175   AKEYCODE_CAPTIONS
        nullptr,                                                // 176   AKEYCODE_SETTINGS
        nullptr,                                                // 177   AKEYCODE_TV_POWER
        nullptr,                                                // 178   AKEYCODE_TV_INPUT
        nullptr,                                                // 179   AKEYCODE_STB_POWER
        nullptr,                                                // 180   AKEYCODE_STB_INPUT
        nullptr,                                                // 181   AKEYCODE_AVR_POWER
        nullptr,                                                // 182   AKEYCODE_AVR_INPUT
        nullptr,                                                // 183   AKEYCODE_PROG_RED
        nullptr,                                                // 184   AKEYCODE_PROG_GREEN
        nullptr,                                                // 185   AKEYCODE_PROG_YELLOW
        nullptr,                                                // 186   AKEYCODE_PROG_BLUE
        nullptr,                                                // 187   AKEYCODE_APP_SWITCH
        nullptr,                                                // 188   AKEYCODE_BUTTON_1
        nullptr,                                                // 189   AKEYCODE_BUTTON_2
        nullptr,                                                // 190   AKEYCODE_BUTTON_3
        nullptr,                                                // 191   AKEYCODE_BUTTON_4
        nullptr,                                                // 192   AKEYCODE_BUTTON_5
        nullptr,                                                // 193   AKEYCODE_BUTTON_6
        nullptr,                                                // 194   AKEYCODE_BUTTON_7
        nullptr,                                                // 195   AKEYCODE_BUTTON_8
        nullptr,                                                // 196   AKEYCODE_BUTTON_9
        nullptr,                                                // 197   AKEYCODE_BUTTON_10
        nullptr,                                                // 198   AKEYCODE_BUTTON_11
        nullptr,                                                // 199   AKEYCODE_BUTTON_12
        nullptr,                                                // 200   AKEYCODE_BUTTON_13
        nullptr,                                                // 201   AKEYCODE_BUTTON_14
        nullptr,                                                // 202   AKEYCODE_BUTTON_15
        nullptr,                                                // 203   AKEYCODE_BUTTON_16
        nullptr,                                                // 204   AKEYCODE_LANGUAGE_SWITCH
        nullptr,                                                // 205   AKEYCODE_MANNER_MODE
        nullptr,                                                // 206   AKEYCODE_3D_MODE
        nullptr,                                                // 207   AKEYCODE_CONTACTS
        nullptr,                                                // 208   AKEYCODE_CALENDAR
        nullptr,                                                // 209   AKEYCODE_MUSIC
        nullptr,                                                // 210   AKEYCODE_CALCULATOR
        nullptr,                                                // 211   AKEYCODE_ZENKAKU_HANKAKU
        nullptr,                                                // 212   AKEYCODE_EISU
        nullptr,                                                // 213   AKEYCODE_MUHENKAN
        nullptr,                                                // 214   AKEYCODE_HENKAN
        nullptr,                                                // 215   AKEYCODE_KATAKANA_HIRAGANA
        nullptr,                                                // 216   AKEYCODE_YEN
        nullptr,                                                // 217   AKEYCODE_RO
        nullptr,                                                // 218   AKEYCODE_KANA
        nullptr,                                                // 219   AKEYCODE_ASSIST
        nullptr,                                                // 220   AKEYCODE_BRIGHTNESS_DOWN
        nullptr,                                                // 221   AKEYCODE_BRIGHTNESS_UP
        nullptr,                                                // 222   AKEYCODE_MEDIA_AUDIO_TRACK
        nullptr,                                                // 223   AKEYCODE_SLEEP
        nullptr,                                                // 224   AKEYCODE_WAKEUP
        nullptr,                                                // 225   AKEYCODE_PAIRING
        nullptr,                                                // 226   AKEYCODE_MEDIA_TOP_MENU
        nullptr,                                                // 227   AKEYCODE_11
        nullptr,                                                // 228   AKEYCODE_12
        nullptr,                                                // 229   AKEYCODE_LAST_CHANNEL
        nullptr,                                                // 230   AKEYCODE_TV_DATA_SERVICE
        nullptr,                                                // 231   AKEYCODE_VOICE_ASSIST
        nullptr,                                                // 232   AKEYCODE_TV_RADIO_SERVICE
        nullptr,                                                // 233   AKEYCODE_TV_TELETEXT
        nullptr,                                                // 234   AKEYCODE_TV_NUMBER_ENTRY
        nullptr,                                                // 235   AKEYCODE_TV_TERRESTRIAL_ANALOG
        nullptr,                                                // 236   AKEYCODE_TV_TERRESTRIAL_DIGITAL
        nullptr,                                                // 237   AKEYCODE_TV_SATELLITE
        nullptr,                                                // 238   AKEYCODE_TV_SATELLITE_BS
        nullptr,                                                // 239   AKEYCODE_TV_SATELLITE_CS
        nullptr,                                                // 240   AKEYCODE_TV_SATELLITE_SERVICE
        nullptr,                                                // 241   AKEYCODE_TV_NETWORK
        nullptr,                                                // 242   AKEYCODE_TV_ANTENNA_CABLE
        nullptr,                                                // 243   AKEYCODE_TV_INPUT_HDMI_1
        nullptr,                                                // 244   AKEYCODE_TV_INPUT_HDMI_2
        nullptr,                                                // 245   AKEYCODE_TV_INPUT_HDMI_3
        nullptr,                                                // 246   AKEYCODE_TV_INPUT_HDMI_4
        nullptr,                                                // 247   AKEYCODE_TV_INPUT_COMPOSITE_1
        nullptr,                                                // 248   AKEYCODE_TV_INPUT_COMPOSITE_2
        nullptr,                                                // 249   AKEYCODE_TV_INPUT_COMPONENT_1
        nullptr,                                                // 250   AKEYCODE_TV_INPUT_COMPONENT_2
        nullptr,                                                // 251   AKEYCODE_TV_INPUT_VGA_1
        nullptr,                                                // 252   AKEYCODE_TV_AUDIO_DESCRIPTION
        nullptr,                                                // 253   AKEYCODE_TV_AUDIO_DESCRIPTION_MIX_UP
        nullptr,                                                // 254   AKEYCODE_TV_AUDIO_DESCRIPTION_MIX_DOWN
        nullptr,                                                // 255   AKEYCODE_TV_ZOOM_MODE
        nullptr,                                                // 256   AKEYCODE_TV_CONTENTS_MENU
        nullptr,                                                // 257   AKEYCODE_TV_MEDIA_CONTEXT_MENU
        nullptr,                                                // 258   AKEYCODE_TV_TIMER_PROGRAMMING
        nullptr,                                                // 259   AKEYCODE_HELP
        nullptr,                                                // 260   AKEYCODE_NAVIGATE_PREVIOUS
        nullptr,                                                // 261   AKEYCODE_NAVIGATE_NEXT
        nullptr,                                                // 262   AKEYCODE_NAVIGATE_IN
        nullptr,                                                // 263   AKEYCODE_NAVIGATE_OUT
        nullptr,                                                // 264   AKEYCODE_STEM_PRIMARY
        nullptr,                                                // 265   AKEYCODE_STEM_1
        nullptr,                                                // 266   AKEYCODE_STEM_2
        nullptr,                                                // 267   AKEYCODE_STEM_3
        nullptr,                                                // 268   AKEYCODE_DPAD_UP_LEFT
        nullptr,                                                // 269   AKEYCODE_DPAD_DOWN_LEFT
        nullptr,                                                // 270   AKEYCODE_DPAD_UP_RIGHT
        nullptr,                                                // 271   AKEYCODE_DPAD_DOWN_RIGHT
        nullptr,                                                // 272   AKEYCODE_MEDIA_SKIP_FORWARD
        nullptr,                                                // 273   AKEYCODE_MEDIA_SKIP_BACKWARD
        nullptr,                                                // 274   AKEYCODE_MEDIA_STEP_FORWARD
        nullptr,                                                // 275   AKEYCODE_MEDIA_STEP_BACKWARD
        nullptr,                                                // 276   AKEYCODE_SOFT_SLEEP
        nullptr,                                                // 277   AKEYCODE_CUT
        nullptr,                                                // 278   AKEYCODE_COPY
        nullptr,                                                // 279   AKEYCODE_PASTE
        nullptr,                                                // 280   AKEYCODE_SYSTEM_NAVIGATION_UP
        nullptr,                                                // 281   AKEYCODE_SYSTEM_NAVIGATION_DOWN
        nullptr,                                                // 282   AKEYCODE_SYSTEM_NAVIGATION_LEFT
        nullptr,                                                // 283   AKEYCODE_SYSTEM_NAVIGATION_RIGHT
        nullptr,                                                // 284   AKEYCODE_ALL_APPS
    }};
}


namespace AzFramework
{
    //! Platform specific implementation for Android physical keyboard input devices.  This
    //! includes devices connected through USB or Bluetooth.  This input device is responsible
    //! for sending key events only, the virtual keyboard is the one responsible for sending
    //! text events. Should this behaviour need to be changed, LY-69260 will correct it
    class InputDeviceKeyboardAndroid
        : public InputDeviceKeyboard::Implementation
        , public RawInputNotificationBusAndroid::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(InputDeviceKeyboardAndroid, AZ::SystemAllocator);


        InputDeviceKeyboardAndroid(InputDeviceKeyboard& inputDevice);
        ~InputDeviceKeyboardAndroid() override;


        //! \ref AzFramework::RawInputNotificationsAndroid::OnRawInputEvent
        void OnRawInputEvent(const AInputEvent* rawInputEvent) override;


    private:
        //! \ref AzFramework::InputDeviceKeyboard::Implementation::IsConnected
        bool IsConnected() const override;

        //! \ref AzFramework::InputDeviceKeyboard::Implementation::HasTextEntryStarted
        bool HasTextEntryStarted() const override;

        //! \ref AzFramework::InputDeviceKeyboard::Implementation::TextEntryStart
        void TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions& options) override;

        //! \ref AzFramework::InputDeviceKeyboard::Implementation::TextEntryStop
        void TextEntryStop() override;

        //! \ref AzFramework::InputDeviceKeyboard::Implementation::TickInputDevice
        void TickInputDevice() override;


        bool m_hasTextEntryStarted = false; //!< Has text entry been started?
    };


    InputDeviceKeyboardAndroid::InputDeviceKeyboardAndroid(InputDeviceKeyboard& inputDevice)
        : InputDeviceKeyboard::Implementation(inputDevice)
        , m_hasTextEntryStarted(false)
    {
        RawInputNotificationBusAndroid::Handler::BusConnect();
    }

    InputDeviceKeyboardAndroid::~InputDeviceKeyboardAndroid()
    {
        RawInputNotificationBusAndroid::Handler::BusDisconnect();
    }


    void InputDeviceKeyboardAndroid::OnRawInputEvent(const AInputEvent* rawInputEvent)
    {
        // don't bother with physical keyboard events if it's not connected
        if (!IsConnected())
        {
            return;
        }

        // only care about key events
        int eventType = AInputEvent_getType(rawInputEvent);
        if (eventType != AINPUT_EVENT_TYPE_KEY)
        {
            return;
        }

        int keyCode = AKeyEvent_getKeyCode(rawInputEvent);
        const InputChannelId* channelId = (keyCode < InputChannelIdByKeyCodeTable.size()) ?
                                          InputChannelIdByKeyCodeTable[keyCode] : nullptr;

        if (channelId)
        {
            int action = AKeyEvent_getAction(rawInputEvent);
            switch (action)
            {
                case AKEY_EVENT_ACTION_DOWN:
                    QueueRawKeyEvent(*channelId, true);
                    break;
                case AKEY_EVENT_ACTION_UP:
                    QueueRawKeyEvent(*channelId, false);
                    break;

                // The multiple event is sent for 2 reasons, both of which we don't care about
                //  1. When the keycode is unknown - a complex string event e.g non-standard english characters
                //  2. When the keycode is known - there were multiple duplicate key events (unlikely)
                case AKEY_EVENT_ACTION_MULTIPLE:
                default:
                    break;
            }
        }
    }


    bool InputDeviceKeyboardAndroid::IsConnected() const
    {
        AConfiguration* config = AZ::Android::Utils::GetConfiguration();
        int keyboard = AConfiguration_getKeyboard(config);
        return (keyboard != ACONFIGURATION_KEYBOARD_NOKEYS); // nokeys == no physical keyboard connected
    }

    bool InputDeviceKeyboardAndroid::HasTextEntryStarted() const
    {
        return m_hasTextEntryStarted;
    }

    void InputDeviceKeyboardAndroid::TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions& options)
    {
        m_hasTextEntryStarted = true;
    }

    void InputDeviceKeyboardAndroid::TextEntryStop()
    {
        m_hasTextEntryStarted = false;
    }

    void InputDeviceKeyboardAndroid::TickInputDevice()
    {
        ProcessRawEventQueues();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class AndroidDeviceKeyboardImplFactory
        : public InputDeviceKeyboard::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceKeyboard::Implementation> Create(InputDeviceKeyboard& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceKeyboardAndroid>(inputDevice);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeDeviceKeyboardImplementationFactory()
    {
        m_deviceKeyboardImplFactory = AZStd::make_unique<AndroidDeviceKeyboardImplFactory>();
        AZ::Interface<InputDeviceKeyboard::ImplementationFactory>::Register(m_deviceKeyboardImplFactory.get());
    }
} // namespace AzFramework
