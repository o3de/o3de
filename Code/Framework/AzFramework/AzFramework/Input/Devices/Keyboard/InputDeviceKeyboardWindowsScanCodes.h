/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Table of key ids indexed by their windows scan code if the E0 'extended bit' prefix isn't set
    const AZStd::array<const InputChannelId*, 128> InputChannelIdByScanCodeTable =
    {{
        nullptr,                                            // 0x00
        &InputDeviceKeyboard::Key::Escape,                  // 0x01
        &InputDeviceKeyboard::Key::Alphanumeric1,           // 0x02
        &InputDeviceKeyboard::Key::Alphanumeric2,           // 0x03
        &InputDeviceKeyboard::Key::Alphanumeric3,           // 0x04
        &InputDeviceKeyboard::Key::Alphanumeric4,           // 0x05
        &InputDeviceKeyboard::Key::Alphanumeric5,           // 0x06
        &InputDeviceKeyboard::Key::Alphanumeric6,           // 0x07
        &InputDeviceKeyboard::Key::Alphanumeric7,           // 0x08
        &InputDeviceKeyboard::Key::Alphanumeric8,           // 0x09
        &InputDeviceKeyboard::Key::Alphanumeric9,           // 0x0A
        &InputDeviceKeyboard::Key::Alphanumeric0,           // 0x0B
        &InputDeviceKeyboard::Key::PunctuationHyphen,       // 0x0C
        &InputDeviceKeyboard::Key::PunctuationEquals,       // 0x0D
        &InputDeviceKeyboard::Key::EditBackspace,           // 0x0E
        &InputDeviceKeyboard::Key::EditTab,                 // 0x0F

        &InputDeviceKeyboard::Key::AlphanumericQ,           // 0x10
        &InputDeviceKeyboard::Key::AlphanumericW,           // 0x11
        &InputDeviceKeyboard::Key::AlphanumericE,           // 0x12
        &InputDeviceKeyboard::Key::AlphanumericR,           // 0x13
        &InputDeviceKeyboard::Key::AlphanumericT,           // 0x14
        &InputDeviceKeyboard::Key::AlphanumericY,           // 0x15
        &InputDeviceKeyboard::Key::AlphanumericU,           // 0x16
        &InputDeviceKeyboard::Key::AlphanumericI,           // 0x17
        &InputDeviceKeyboard::Key::AlphanumericO,           // 0x18
        &InputDeviceKeyboard::Key::AlphanumericP,           // 0x19
        &InputDeviceKeyboard::Key::PunctuationBracketL,     // 0x1A
        &InputDeviceKeyboard::Key::PunctuationBracketR,     // 0x1B
        &InputDeviceKeyboard::Key::EditEnter,               // 0x1C
        &InputDeviceKeyboard::Key::ModifierCtrlL,           // 0x1D
        &InputDeviceKeyboard::Key::AlphanumericA,           // 0x1E
        &InputDeviceKeyboard::Key::AlphanumericS,           // 0x1F

        &InputDeviceKeyboard::Key::AlphanumericD,           // 0x20
        &InputDeviceKeyboard::Key::AlphanumericF,           // 0x21
        &InputDeviceKeyboard::Key::AlphanumericG,           // 0x22
        &InputDeviceKeyboard::Key::AlphanumericH,           // 0x23
        &InputDeviceKeyboard::Key::AlphanumericJ,           // 0x24
        &InputDeviceKeyboard::Key::AlphanumericK,           // 0x25
        &InputDeviceKeyboard::Key::AlphanumericL,           // 0x26
        &InputDeviceKeyboard::Key::PunctuationSemicolon,    // 0x27
        &InputDeviceKeyboard::Key::PunctuationApostrophe,   // 0x28
        &InputDeviceKeyboard::Key::PunctuationTilde,        // 0x29
        &InputDeviceKeyboard::Key::ModifierShiftL,          // 0x2A
        &InputDeviceKeyboard::Key::PunctuationBackslash,    // 0x2B
        &InputDeviceKeyboard::Key::AlphanumericZ,           // 0x2C
        &InputDeviceKeyboard::Key::AlphanumericX,           // 0x2D
        &InputDeviceKeyboard::Key::AlphanumericC,           // 0x2E
        &InputDeviceKeyboard::Key::AlphanumericV,           // 0x2F
        
        &InputDeviceKeyboard::Key::AlphanumericB,           // 0x30
        &InputDeviceKeyboard::Key::AlphanumericN,           // 0x31
        &InputDeviceKeyboard::Key::AlphanumericM,           // 0x32
        &InputDeviceKeyboard::Key::PunctuationComma,        // 0x33
        &InputDeviceKeyboard::Key::PunctuationPeriod,       // 0x34
        &InputDeviceKeyboard::Key::PunctuationSlash,        // 0x35
        &InputDeviceKeyboard::Key::ModifierShiftR,          // 0x36
        &InputDeviceKeyboard::Key::NumPadMultiply,          // 0x37
        &InputDeviceKeyboard::Key::ModifierAltL,            // 0x38
        &InputDeviceKeyboard::Key::EditSpace,               // 0x39
        &InputDeviceKeyboard::Key::EditCapsLock,            // 0x3A
        &InputDeviceKeyboard::Key::Function01,              // 0x3B
        &InputDeviceKeyboard::Key::Function02,              // 0x3C
        &InputDeviceKeyboard::Key::Function03,              // 0x3D
        &InputDeviceKeyboard::Key::Function04,              // 0x3E
        &InputDeviceKeyboard::Key::Function05,              // 0x3F

        &InputDeviceKeyboard::Key::Function06,              // 0x40
        &InputDeviceKeyboard::Key::Function07,              // 0x41
        &InputDeviceKeyboard::Key::Function08,              // 0x42
        &InputDeviceKeyboard::Key::Function09,              // 0x43
        &InputDeviceKeyboard::Key::Function10,              // 0x44
        &InputDeviceKeyboard::Key::NumLock,                 // 0x45
        &InputDeviceKeyboard::Key::WindowsSystemScrollLock, // 0x46
        &InputDeviceKeyboard::Key::NumPad7,                 // 0x47
        &InputDeviceKeyboard::Key::NumPad8,                 // 0x48
        &InputDeviceKeyboard::Key::NumPad9,                 // 0x49
        &InputDeviceKeyboard::Key::NumPadSubtract,          // 0x4A
        &InputDeviceKeyboard::Key::NumPad4,                 // 0x4B
        &InputDeviceKeyboard::Key::NumPad5,                 // 0x4C
        &InputDeviceKeyboard::Key::NumPad6,                 // 0x4D
        &InputDeviceKeyboard::Key::NumPadAdd,               // 0x4E
        &InputDeviceKeyboard::Key::NumPad1,                 // 0x4F
  
        &InputDeviceKeyboard::Key::NumPad2,                 // 0x50
        &InputDeviceKeyboard::Key::NumPad3,                 // 0x51
        &InputDeviceKeyboard::Key::NumPad0,                 // 0x52
        &InputDeviceKeyboard::Key::NumPadDecimal,           // 0x53
        nullptr, // Sys Req?                                // 0x54
        nullptr,                                            // 0x55
        &InputDeviceKeyboard::Key::SupplementaryISO,        // 0x56
        &InputDeviceKeyboard::Key::Function11,              // 0x57
        &InputDeviceKeyboard::Key::Function12,              // 0x58
        &InputDeviceKeyboard::Key::WindowsSystemPause,      // 0x59
        nullptr,                                            // 0x5A
        &InputDeviceKeyboard::Key::ModifierSuperL,          // 0x5B
        &InputDeviceKeyboard::Key::ModifierSuperR,          // 0x5C
        nullptr,                                            // 0x5D
        nullptr,                                            // 0x5E
        nullptr,                                            // 0x5F

        nullptr,                                            // 0x60
        nullptr,                                            // 0x61
        nullptr,                                            // 0x62
        nullptr,                                            // 0x63
        &InputDeviceKeyboard::Key::Function13,              // 0x64
        &InputDeviceKeyboard::Key::Function14,              // 0x65
        &InputDeviceKeyboard::Key::Function15,              // 0x66
        &InputDeviceKeyboard::Key::Function16,              // 0x67
        &InputDeviceKeyboard::Key::Function17,              // 0x68
        &InputDeviceKeyboard::Key::Function18,              // 0x69
        &InputDeviceKeyboard::Key::Function19,              // 0x6A
        nullptr,                                            // 0x6B
        nullptr,                                            // 0x6C
        nullptr,                                            // 0x6D
        nullptr,                                            // 0x6E
        nullptr,                                            // 0x6F

        nullptr,                                            // 0x70
        nullptr,                                            // 0x71
        nullptr,                                            // 0x72
        nullptr,                                            // 0x73
        nullptr,                                            // 0x74
        nullptr,                                            // 0x75
        nullptr,                                            // 0x76
        nullptr,                                            // 0x77
        nullptr,                                            // 0x78
        nullptr,                                            // 0x79
        nullptr,                                            // 0x7A
        nullptr,                                            // 0x7B
        nullptr,                                            // 0x7C
        nullptr,                                            // 0x7D
        nullptr,                                            // 0x7E
        nullptr                                             // 0x7F
    }};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Table of key ids indexed by their windows scan code if the E0 'extended bit' prefix is set
    const AZStd::array<const InputChannelId*, 128> InputChannelIdByScanCodeWithExtendedPrefixTable =
    {{
        nullptr,                                            // 0x00
        nullptr,                                            // 0x01
        nullptr,                                            // 0x02
        nullptr,                                            // 0x03
        nullptr,                                            // 0x04
        nullptr,                                            // 0x05
        nullptr,                                            // 0x06
        nullptr,                                            // 0x07
        nullptr,                                            // 0x08
        nullptr,                                            // 0x09
        nullptr,                                            // 0x0A
        nullptr,                                            // 0x0B
        nullptr,                                            // 0x0C
        nullptr,                                            // 0x0D
        nullptr,                                            // 0x0E
        nullptr,                                            // 0x0F

        nullptr,                                            // 0x10
        nullptr,                                            // 0x11
        nullptr,                                            // 0x12
        nullptr,                                            // 0x13
        nullptr,                                            // 0x14
        nullptr,                                            // 0x15
        nullptr,                                            // 0x16
        nullptr,                                            // 0x17
        nullptr,                                            // 0x18
        nullptr,                                            // 0x19
        nullptr,                                            // 0x1A
        nullptr,                                            // 0x1B
        &InputDeviceKeyboard::Key::NumPadEnter,             // 0x1C
        &InputDeviceKeyboard::Key::ModifierCtrlR,           // 0x1D
        nullptr,                                            // 0x1E
        nullptr,                                            // 0x1F

        nullptr,                                            // 0x20
        nullptr,                                            // 0x21
        nullptr,                                            // 0x22
        nullptr,                                            // 0x23
        nullptr,                                            // 0x24
        nullptr,                                            // 0x25
        nullptr,                                            // 0x26
        nullptr,                                            // 0x27
        nullptr,                                            // 0x28
        nullptr,                                            // 0x29
        &InputDeviceKeyboard::Key::WindowsSystemPrint,      // 0x2A
        nullptr,                                            // 0x2B
        nullptr,                                            // 0x2C
        nullptr,                                            // 0x2D
        nullptr,                                            // 0x2E
        nullptr,                                            // 0x2F
        
        nullptr,                                            // 0x30
        nullptr,                                            // 0x31
        nullptr,                                            // 0x32
        nullptr,                                            // 0x33
        nullptr,                                            // 0x34
        &InputDeviceKeyboard::Key::NumPadDivide,            // 0x35
        nullptr,                                            // 0x36
        &InputDeviceKeyboard::Key::WindowsSystemPrint,      // 0x37
        &InputDeviceKeyboard::Key::ModifierAltR,            // 0x38
        nullptr,                                            // 0x39
        nullptr,                                            // 0x3A
        nullptr,                                            // 0x3B
        nullptr,                                            // 0x3C
        nullptr,                                            // 0x3D
        nullptr,                                            // 0x3E
        nullptr,                                            // 0x3F

        nullptr,                                            // 0x40
        nullptr,                                            // 0x41
        nullptr,                                            // 0x42
        nullptr,                                            // 0x43
        nullptr,                                            // 0x44
        nullptr,                                            // 0x45
        nullptr,                                            // 0x46
        &InputDeviceKeyboard::Key::NavigationHome,          // 0x47
        &InputDeviceKeyboard::Key::NavigationArrowUp,       // 0x48
        &InputDeviceKeyboard::Key::NavigationPageUp,        // 0x49
        nullptr,                                            // 0x4A
        &InputDeviceKeyboard::Key::NavigationArrowLeft,     // 0x4B
        nullptr,                                            // 0x4C
        &InputDeviceKeyboard::Key::NavigationArrowRight,    // 0x4D
        nullptr,                                            // 0x4E
        &InputDeviceKeyboard::Key::NavigationEnd,           // 0x4F
  
        &InputDeviceKeyboard::Key::NavigationArrowDown,     // 0x50
        &InputDeviceKeyboard::Key::NavigationPageDown,      // 0x51
        &InputDeviceKeyboard::Key::NavigationInsert,        // 0x52
        &InputDeviceKeyboard::Key::NavigationDelete,        // 0x53
        nullptr,                                            // 0x54
        nullptr,                                            // 0x55
        nullptr,                                            // 0x56
        nullptr,                                            // 0x57
        nullptr,                                            // 0x58
        nullptr,                                            // 0x59
        nullptr,                                            // 0x5A
        nullptr,                                            // 0x5B
        nullptr,                                            // 0x5C
        nullptr,                                            // 0x5D
        nullptr,                                            // 0x5E
        nullptr,                                            // 0x5F

        nullptr,                                            // 0x60
        nullptr,                                            // 0x61
        nullptr,                                            // 0x62
        nullptr,                                            // 0x63
        nullptr,                                            // 0x64
        nullptr,                                            // 0x65
        nullptr,                                            // 0x66
        nullptr,                                            // 0x67
        nullptr,                                            // 0x68
        nullptr,                                            // 0x69
        nullptr,                                            // 0x6A
        nullptr,                                            // 0x6B
        nullptr,                                            // 0x6C
        nullptr,                                            // 0x6D
        nullptr,                                            // 0x6E
        nullptr,                                            // 0x6F

        nullptr,                                            // 0x70
        nullptr,                                            // 0x71
        nullptr,                                            // 0x72
        nullptr,                                            // 0x73
        nullptr,                                            // 0x74
        nullptr,                                            // 0x75
        nullptr,                                            // 0x76
        nullptr,                                            // 0x77
        nullptr,                                            // 0x78
        nullptr,                                            // 0x79
        nullptr,                                            // 0x7A
        nullptr,                                            // 0x7B
        nullptr,                                            // 0x7C
        nullptr,                                            // 0x7D
        nullptr,                                            // 0x7E
        nullptr                                             // 0x7F
    }};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Table of key ids indexed by their virtual key code. This should only be used as a last resort
    // if the input channel id cannot be determined directly from the scan code, as some scan codes
    // generate the same virtual key code (eg. the 'enter' and 'numpad enter' keys), or can produce
    // different virtual key codes depending on the state of other keys (eg. some of the number pad
    // keys produce different virtual key codes depending if the numlock key is toggled on or off).
    //
    // One exception is the system pause key, which generates the same scan code as the numlock key,
    // meaning we must check the virtual key code to be able to distinguish between these two keys.
    const AZStd::array<const InputChannelId*, 255> InputChannelIdByVirtualKeyCodeTable =
    {{
        nullptr,                                            // 0x00
        nullptr,                                            // 0x01 VK_LBUTTON
        nullptr,                                            // 0x02 VK_RBUTTON
        nullptr,                                            // 0x03 VK_CANCEL
        nullptr,                                            // 0x04 VK_MBUTTON
        nullptr,                                            // 0x05 VK_XBUTTON1
        nullptr,                                            // 0x06 VK_XBUTTON2
        nullptr,                                            // 0x07
        &InputDeviceKeyboard::Key::EditBackspace,           // 0x08 VK_BACK
        &InputDeviceKeyboard::Key::EditTab,                 // 0x09 VK_TAB
        nullptr,                                            // 0x0A
        nullptr,                                            // 0x0B
        nullptr,                                            // 0x0C VK_CLEAR
        &InputDeviceKeyboard::Key::EditEnter,               // 0x0D VK_RETURN
        nullptr,                                            // 0x0E
        nullptr,                                            // 0x0F

        nullptr,                                            // 0x10 VK_SHIFT
        nullptr,                                            // 0x11 VK_CONTROL
        nullptr,                                            // 0x12 VK_MENU
        &InputDeviceKeyboard::Key::WindowsSystemPause,      // 0x13 VK_PAUSE
        &InputDeviceKeyboard::Key::EditCapsLock,            // 0x14 VK_CAPITAL
        nullptr,                                            // 0x15 VK_KANA
        nullptr,                                            // 0x16
        nullptr,                                            // 0x17 VK_JUNJA
        nullptr,                                            // 0x18 VK_FINAL
        nullptr,                                            // 0x19 VK_KANJI
        nullptr,                                            // 0x1A
        &InputDeviceKeyboard::Key::Escape,                  // 0x1B VK_ESCAPE
        nullptr,                                            // 0x1C VK_CONVERT
        nullptr,                                            // 0x1D VK_NONCONVERT
        nullptr,                                            // 0x1E VK_ACCEPT
        nullptr,                                            // 0x1F VK_MODECHANGE

        &InputDeviceKeyboard::Key::EditSpace,               // 0x20 VK_SPACE
        &InputDeviceKeyboard::Key::NavigationPageUp,        // 0x21 VK_PRIOR
        &InputDeviceKeyboard::Key::NavigationPageDown,      // 0x22 VK_NEXT
        &InputDeviceKeyboard::Key::NavigationEnd,           // 0x23 VK_END
        &InputDeviceKeyboard::Key::NavigationHome,          // 0x24 VK_HOME
        &InputDeviceKeyboard::Key::NavigationArrowLeft,     // 0x25 VK_LEFT
        &InputDeviceKeyboard::Key::NavigationArrowUp,       // 0x26 VK_UP
        &InputDeviceKeyboard::Key::NavigationArrowRight,    // 0x27 VK_RIGHT
        &InputDeviceKeyboard::Key::NavigationArrowDown,     // 0x28 VK_DOWN
        nullptr,                                            // 0x29 VK_SELECT
        nullptr,                                            // 0x2A VK_PRINT
        nullptr,                                            // 0x2B VK_EXECUTE
        &InputDeviceKeyboard::Key::WindowsSystemPrint,      // 0x2C VK_SNAPSHOT
        &InputDeviceKeyboard::Key::NavigationInsert,        // 0x2D VK_INSERT
        &InputDeviceKeyboard::Key::NavigationDelete,        // 0x2E VK_DELETE
        nullptr,                                            // 0x2F VK_HELP

        &InputDeviceKeyboard::Key::Alphanumeric0,           // 0x30
        &InputDeviceKeyboard::Key::Alphanumeric1,           // 0x31
        &InputDeviceKeyboard::Key::Alphanumeric2,           // 0x32
        &InputDeviceKeyboard::Key::Alphanumeric3,           // 0x33
        &InputDeviceKeyboard::Key::Alphanumeric4,           // 0x34
        &InputDeviceKeyboard::Key::Alphanumeric5,           // 0x35
        &InputDeviceKeyboard::Key::Alphanumeric6,           // 0x36
        &InputDeviceKeyboard::Key::Alphanumeric7,           // 0x37
        &InputDeviceKeyboard::Key::Alphanumeric8,           // 0x38
        &InputDeviceKeyboard::Key::Alphanumeric9,           // 0x39
        nullptr,                                            // 0x3A
        nullptr,                                            // 0x3B
        nullptr,                                            // 0x3C
        nullptr,                                            // 0x3D
        nullptr,                                            // 0x3E
        nullptr,                                            // 0x3F

        nullptr,                                            // 0x40
        &InputDeviceKeyboard::Key::AlphanumericA,           // 0x41
        &InputDeviceKeyboard::Key::AlphanumericB,           // 0x42
        &InputDeviceKeyboard::Key::AlphanumericC,           // 0x43
        &InputDeviceKeyboard::Key::AlphanumericD,           // 0x44
        &InputDeviceKeyboard::Key::AlphanumericE,           // 0x45
        &InputDeviceKeyboard::Key::AlphanumericF,           // 0x46
        &InputDeviceKeyboard::Key::AlphanumericG,           // 0x47
        &InputDeviceKeyboard::Key::AlphanumericH,           // 0x48
        &InputDeviceKeyboard::Key::AlphanumericI,           // 0x49
        &InputDeviceKeyboard::Key::AlphanumericJ,           // 0x4A
        &InputDeviceKeyboard::Key::AlphanumericK,           // 0x4B
        &InputDeviceKeyboard::Key::AlphanumericL,           // 0x4C
        &InputDeviceKeyboard::Key::AlphanumericM,           // 0x4D
        &InputDeviceKeyboard::Key::AlphanumericN,           // 0x4E
        &InputDeviceKeyboard::Key::AlphanumericO,           // 0x4F

        &InputDeviceKeyboard::Key::AlphanumericP,           // 0x50
        &InputDeviceKeyboard::Key::AlphanumericQ,           // 0x51
        &InputDeviceKeyboard::Key::AlphanumericR,           // 0x52
        &InputDeviceKeyboard::Key::AlphanumericS,           // 0x53
        &InputDeviceKeyboard::Key::AlphanumericT,           // 0x54
        &InputDeviceKeyboard::Key::AlphanumericU,           // 0x55
        &InputDeviceKeyboard::Key::AlphanumericV,           // 0x56
        &InputDeviceKeyboard::Key::AlphanumericW,           // 0x57
        &InputDeviceKeyboard::Key::AlphanumericX,           // 0x58
        &InputDeviceKeyboard::Key::AlphanumericY,           // 0x59
        &InputDeviceKeyboard::Key::AlphanumericZ,           // 0x5A
        &InputDeviceKeyboard::Key::ModifierSuperL,          // 0x5B VK_LWIN
        &InputDeviceKeyboard::Key::ModifierSuperR,          // 0x5C VK_RWIN
        nullptr,                                            // 0x5D VK_APPS
        nullptr,                                            // 0x5E
        nullptr,                                            // 0x5F VK_SLEEP

        &InputDeviceKeyboard::Key::NumPad0,                 // 0x60 VK_NUMPAD0
        &InputDeviceKeyboard::Key::NumPad1,                 // 0x61 VK_NUMPAD1
        &InputDeviceKeyboard::Key::NumPad2,                 // 0x62 VK_NUMPAD2
        &InputDeviceKeyboard::Key::NumPad3,                 // 0x63 VK_NUMPAD3
        &InputDeviceKeyboard::Key::NumPad4,                 // 0x64 VK_NUMPAD4
        &InputDeviceKeyboard::Key::NumPad5,                 // 0x65 VK_NUMPAD5
        &InputDeviceKeyboard::Key::NumPad6,                 // 0x66 VK_NUMPAD6
        &InputDeviceKeyboard::Key::NumPad7,                 // 0x67 VK_NUMPAD7
        &InputDeviceKeyboard::Key::NumPad8,                 // 0x68 VK_NUMPAD8
        &InputDeviceKeyboard::Key::NumPad9,                 // 0x69 VK_NUMPAD9
        &InputDeviceKeyboard::Key::NumPadMultiply,          // 0x6A VK_MULTIPLY
        &InputDeviceKeyboard::Key::NumPadAdd,               // 0x6B VK_ADD
        &InputDeviceKeyboard::Key::NumPadEnter,             // 0x6C VK_SEPARATOR
        &InputDeviceKeyboard::Key::NumPadSubtract,          // 0x6D VK_SUBTRACT
        &InputDeviceKeyboard::Key::NumPadDecimal,           // 0x6E VK_DECIMAL
        &InputDeviceKeyboard::Key::NumPadDivide,            // 0x6F VK_DIVIDE

        &InputDeviceKeyboard::Key::Function01,              // 0x70 VK_F1
        &InputDeviceKeyboard::Key::Function02,              // 0x71 VK_F2
        &InputDeviceKeyboard::Key::Function03,              // 0x72 VK_F3
        &InputDeviceKeyboard::Key::Function04,              // 0x73 VK_F4
        &InputDeviceKeyboard::Key::Function05,              // 0x74 VK_F5
        &InputDeviceKeyboard::Key::Function06,              // 0x75 VK_F6
        &InputDeviceKeyboard::Key::Function07,              // 0x76 VK_F7
        &InputDeviceKeyboard::Key::Function08,              // 0x77 VK_F8
        &InputDeviceKeyboard::Key::Function09,              // 0x78 VK_F9
        &InputDeviceKeyboard::Key::Function10,              // 0x79 VK_F10
        &InputDeviceKeyboard::Key::Function11,              // 0x7A VK_F11
        &InputDeviceKeyboard::Key::Function12,              // 0x7B VK_F12
        &InputDeviceKeyboard::Key::Function13,              // 0x7C VK_F13
        &InputDeviceKeyboard::Key::Function14,              // 0x7D VK_F14
        &InputDeviceKeyboard::Key::Function15,              // 0x7E VK_F15
        &InputDeviceKeyboard::Key::Function16,              // 0x7F VK_F16

        &InputDeviceKeyboard::Key::Function17,              // 0x80 VK_F17
        &InputDeviceKeyboard::Key::Function18,              // 0x81 VK_F18
        &InputDeviceKeyboard::Key::Function19,              // 0x82 VK_F19
        &InputDeviceKeyboard::Key::Function20,              // 0x83 VK_F20
        nullptr,                                            // 0x84 VK_F21
        nullptr,                                            // 0x85 VK_F22
        nullptr,                                            // 0x86 VK_F23
        nullptr,                                            // 0x87 VK_F24
        nullptr,                                            // 0x88
        nullptr,                                            // 0x89
        nullptr,                                            // 0x8A
        nullptr,                                            // 0x8B
        nullptr,                                            // 0x8C
        nullptr,                                            // 0x8D
        nullptr,                                            // 0x8E
        nullptr,                                            // 0x8F

        &InputDeviceKeyboard::Key::NumLock,                 // 0x90 VK_NUMLOCK
        &InputDeviceKeyboard::Key::WindowsSystemScrollLock, // 0x91 VK_SCROLL
        nullptr,                                            // 0x92
        nullptr,                                            // 0x93
        nullptr,                                            // 0x94
        nullptr,                                            // 0x95
        nullptr,                                            // 0x96
        nullptr,                                            // 0x97
        nullptr,                                            // 0x98
        nullptr,                                            // 0x99
        nullptr,                                            // 0x9A
        nullptr,                                            // 0x9B
        nullptr,                                            // 0x9C
        nullptr,                                            // 0x9D
        nullptr,                                            // 0x9E
        nullptr,                                            // 0x9F

        &InputDeviceKeyboard::Key::ModifierShiftL,          // 0xA0 VK_LSHIFT
        &InputDeviceKeyboard::Key::ModifierShiftR,          // 0xA1 VK_RSHIFT
        &InputDeviceKeyboard::Key::ModifierCtrlL,           // 0xA2 VK_LCONTROL
        &InputDeviceKeyboard::Key::ModifierCtrlR,           // 0xA3 VK_RCONTROL
        &InputDeviceKeyboard::Key::ModifierAltL,            // 0xA4 VK_LMENU
        &InputDeviceKeyboard::Key::ModifierAltR,            // 0xA5 VK_RMENU
        nullptr,                                            // 0xA6 VK_BROWSER_BACK
        nullptr,                                            // 0xA7 VK_BROWSER_FORWARD
        nullptr,                                            // 0xA8 VK_BROWSER_REFRESH
        nullptr,                                            // 0xA9 VK_BROWSER_STOP
        nullptr,                                            // 0xAA VK_BROWSER_SEARCH
        nullptr,                                            // 0xAB VK_BROWSER_FAVORITES
        nullptr,                                            // 0xAC VK_BROWSER_HOME
        nullptr,                                            // 0xAD VK_VOLUME_MUTE
        nullptr,                                            // 0xAE VK_VOLUME_DOWN
        nullptr,                                            // 0xAF VK_VOLUME_UP

        nullptr,                                            // 0xB0 VK_MEDIA_NEXT_TRACK
        nullptr,                                            // 0xB1 VK_MEDIA_PREV_TRACK
        nullptr,                                            // 0xB2 VK_MEDIA_STOP
        nullptr,                                            // 0xB3 VK_MEDIA_PLAY_PAUSE
        nullptr,                                            // 0xB4 VK_LAUNCH_MAIL
        nullptr,                                            // 0xB5 VK_LAUNCH_MEDIA_SELECT
        nullptr,                                            // 0xB6 VK_LAUNCH_APP1
        nullptr,                                            // 0xB7 VK_LAUNCH_APP2
        nullptr,                                            // 0xB8
        nullptr,                                            // 0xB9
        &InputDeviceKeyboard::Key::PunctuationSemicolon,    // 0xBA VK_OEM_1
        &InputDeviceKeyboard::Key::PunctuationEquals,       // 0xBB VK_OEM_PLUS
        &InputDeviceKeyboard::Key::PunctuationComma,        // 0xBC VK_OEM_COMMA
        &InputDeviceKeyboard::Key::PunctuationHyphen,       // 0xBD VK_OEM_MINUS
        &InputDeviceKeyboard::Key::PunctuationPeriod,       // 0xBE VK_OEM_PERIOD
        &InputDeviceKeyboard::Key::PunctuationSlash,        // 0xBF VK_OEM_2

        &InputDeviceKeyboard::Key::PunctuationTilde,        // 0xC0 VK_OEM_3
        nullptr,                                            // 0xC1
        nullptr,                                            // 0xC2
        nullptr,                                            // 0xC3
        nullptr,                                            // 0xC4
        nullptr,                                            // 0xC5
        nullptr,                                            // 0xC6
        nullptr,                                            // 0xC7
        nullptr,                                            // 0xC8
        nullptr,                                            // 0xC9
        nullptr,                                            // 0xCA
        nullptr,                                            // 0xCB
        nullptr,                                            // 0xCC
        nullptr,                                            // 0xCD
        nullptr,                                            // 0xCE
        nullptr,                                            // 0xCF

        nullptr,                                            // 0xD0
        nullptr,                                            // 0xD1
        nullptr,                                            // 0xD2
        nullptr,                                            // 0xD3
        nullptr,                                            // 0xD4
        nullptr,                                            // 0xD5
        nullptr,                                            // 0xD6
        nullptr,                                            // 0xD7
        nullptr,                                            // 0xD8
        nullptr,                                            // 0xD9
        nullptr,                                            // 0xDA
        &InputDeviceKeyboard::Key::PunctuationBracketL,     // 0xDB VK_OEM_4
        &InputDeviceKeyboard::Key::PunctuationBackslash,    // 0xDC VK_OEM_5
        &InputDeviceKeyboard::Key::PunctuationBracketR,     // 0xDD VK_OEM_6
        &InputDeviceKeyboard::Key::PunctuationApostrophe,   // 0xDE VK_OEM_7
        nullptr,                                            // 0xDF VK_OEM_8

        nullptr,                                            // 0xE0
        nullptr,                                            // 0xE1
        &InputDeviceKeyboard::Key::SupplementaryISO,        // 0xE2 VK_OEM_102
        nullptr,                                            // 0xE3
        nullptr,                                            // 0xE4
        nullptr,                                            // 0xE5 VK_PROCESSKEY
        nullptr,                                            // 0xE6
        nullptr,                                            // 0xE7 VK_PACKET
        nullptr,                                            // 0xE8
        nullptr,                                            // 0xE9
        nullptr,                                            // 0xEA
        nullptr,                                            // 0xEB
        nullptr,                                            // 0xEC
        nullptr,                                            // 0xED
        nullptr,                                            // 0xEE
        nullptr,                                            // 0xEF

        nullptr,                                            // 0xF0
        nullptr,                                            // 0xF1
        nullptr,                                            // 0xF2
        nullptr,                                            // 0xF3
        nullptr,                                            // 0xF4
        nullptr,                                            // 0xF5
        nullptr,                                            // 0xF6 VK_ATTN
        nullptr,                                            // 0xF7 VK_CRSEL
        nullptr,                                            // 0xF8 VK_EXSEL
        nullptr,                                            // 0xF9 VK_EREOF
        nullptr,                                            // 0xFA VK_PLAY
        nullptr,                                            // 0xFB VK_ZOOM
        nullptr,                                            // 0xFC VK_NONAME
        nullptr,                                            // 0xFD VK_PA1
        nullptr                                             // 0xFE VK_OEM_CLEAR
    }};
}
