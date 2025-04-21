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

#include <AppKit/NSEvent.h>
#include <AppKit/NSView.h>
#include <AppKit/NSWindow.h>
#include <Carbon/Carbon.h>

#include <objc/runtime.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Ideally this would all be done by simply sub-classing NSTextView, but we've been forced to resort
// to these shenanigans because the objective-c runtime deems a class defined in a static lib should
// generate the following warning if that static lib happens to be used by multiple dynamic libs:
//
// "Class X is implemented in both A and B. One of the two will be used. Which one is undefined."
//
// To get around this absurdity we're using method swizzling to hook into two NSResponder methods we
// can then customize by using EBus to forward them to our InputDeviceKeyboardMac instance, which in
// turn will either intercept the calls to implement our custom logic if it owns the NSResponder, or
// return false if it does not own the NSResponder so that we'll invoke the original implementation.
//
// One advantage to all this is that we don't need to add anything to the view hierarchy, instead we
// just create a dummy NSView and call makeFirstResponder then interpretKeyEvents process text input.
namespace
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Sets the implementation of an objectice-c instance method
    //! \param[in] selector The selector used to invoke the method
    //! \param[in] newImplementation The new implementation of the method to set
    //! \return The existing implementation if it differs from newImplementation, nullptr otherwise
    IMP SetInstanceMethodImplementaion(Class classType, SEL selector, IMP newImplementation)
    {
        Method instanceMethod = class_getInstanceMethod(classType, selector);
        if (!instanceMethod)
        {
            AZ_Warning("SetInstanceMethodImplementaion", false, "Instance method not found");
            return nullptr;
        }
        
        if (method_getImplementation(instanceMethod) == newImplementation)
        {
            return nullptr;
        }

        return method_setImplementation(instanceMethod, newImplementation);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for NSResponder method calls that have been intercepted
    class NSResponderMethodHookNotifications : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: NSResponder method hook notifications are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: NSResponder method hook notifications can be handled by multiple listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Sent when a call to the NSResponder::insertText method has been intercepted
        //! \param[in] responder The NSResponder that the original method was invoked on
        //! \param[in] textString The text to insert (could be an NString or an NSAttributedString)
        //! \return True if the method call was handled, false otherwise
        virtual bool InsertText(NSResponder* responder, id textString) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Sent when a call to the NSResponder::noResponderFor method has been intercepted
        //! \param[in] responder The NSResponder that the original method was invoked on
        //! \param[in] eventSelector The event selector that was sent to the original message
        //! \return True if the method call was handled, false otherwise
        virtual bool NoResponderFor(NSResponder* responder, SEL eventSelector) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Sent when a call to the NSResponder::doCommandBySelector method has been intercepted
        //! \param[in] responder The NSResponder that the original method was invoked on
        //! \param[in] commandSelector The command selector that was sent to the original message
        //! \return True if the method call was handled, false otherwise
        virtual bool DoCommandBySelector(NSResponder* responder, SEL commandSelector) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Call to insert the custom method hooks into the NSResponder class implementation
        static void InsertHooks();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Call to remove the custom method hooks from the NSResponder class implementation
        static void RemoveHooks();

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Custom implementation of NSResponder::insertText:
        //! \ref NSResponder::insertText:
        static void InsertTextHook(NSResponder* responder,
                                   SEL methodSelector,
                                   id textString);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Custom implementation of NSResponder::noResponderFor:
        //! \ref NSResponder::noResponderFor:
        static void NoResponderForHook(NSResponder* responder,
                                       SEL methodSelector,
                                       SEL eventSelector);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Custom implementation of NSResponder::doCommandBySelector:
        //! \ref NSResponder::doCommandBySelector:
        static void DoCommandBySelectorHook(NSResponder* responder,
                                            SEL methodSelector,
                                            SEL commandSelector);

        using imp_redirector = void (*)(NSResponder* responder, SEL methodSelector,id textString);
        using cmd_redirector = void (*)(NSResponder* responder, SEL previousSelector,SEL newSelector);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Pointer to the default implementation of the NSResponder::insertText method
        static IMP s_defaultInsertTextImplementation;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Pointer to the default implementation of the NSResponder::noResponderFor method
        static IMP s_defaultNoResponderForImplementation;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Pointer to the default implementation of the NSResponder::doCommandBySelector method
        static IMP s_defaultDoCommandBySelectorImplementation;
    };
    using NSResponderMethodHookNotificationBus = AZ::EBus<NSResponderMethodHookNotifications>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    IMP NSResponderMethodHookNotifications::s_defaultInsertTextImplementation = nullptr;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    IMP NSResponderMethodHookNotifications::s_defaultNoResponderForImplementation = nullptr;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    IMP NSResponderMethodHookNotifications::s_defaultDoCommandBySelectorImplementation = nullptr;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NSResponderMethodHookNotifications::InsertHooks()
    {
        // Switch the imlplementation of NSResponder::insertText with our custom one,
        // storing the original default implementation so that it can be restored later.
        s_defaultInsertTextImplementation =
            SetInstanceMethodImplementaion([NSResponder class],
                                           @selector(insertText:),
                                           (IMP)InsertTextHook);

        // Switch the imlplementation of NSResponder::noResponderFor with our custom one,
        // storing the original default implementation so that it can be restored later.
        s_defaultNoResponderForImplementation =
            SetInstanceMethodImplementaion([NSResponder class],
                                           @selector(noResponderFor:),
                                           (IMP)NoResponderForHook);

        // Switch the imlplementation of NSResponder::doCommandBySelector with our custom one,
        // storing the original default implementation so that it can be restored later.
        s_defaultDoCommandBySelectorImplementation =
            SetInstanceMethodImplementaion([NSResponder class],
                                           @selector(doCommandBySelector:),
                                           (IMP)DoCommandBySelectorHook);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NSResponderMethodHookNotifications::RemoveHooks()
    {
        // Restore the default imlplementation of NSResponder::doCommandBySelector.
        SetInstanceMethodImplementaion([NSResponder class],
                                       @selector(doCommandBySelector:),
                                       s_defaultDoCommandBySelectorImplementation);
        s_defaultDoCommandBySelectorImplementation = nullptr;

        // Restore the default imlplementation of NSResponder::noResponderFor.
        SetInstanceMethodImplementaion([NSResponder class],
                                       @selector(noResponderFor:),
                                       s_defaultNoResponderForImplementation);
        s_defaultNoResponderForImplementation = nullptr;

        // Restore the default imlplementation of NSResponder::insertText.
        SetInstanceMethodImplementaion([NSResponder class],
                                       @selector(insertText:),
                                       s_defaultInsertTextImplementation);
        s_defaultInsertTextImplementation = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NSResponderMethodHookNotifications::InsertTextHook(NSResponder* responder,
                                                            SEL methodSelector,
                                                            id textString)
    {
        bool handled = false;
        NSResponderMethodHookNotificationBus::BroadcastResult(
            handled,
            &NSResponderMethodHookNotifications::InsertText,
            responder,
            textString);

        if (!handled && s_defaultInsertTextImplementation)
        {
            reinterpret_cast<imp_redirector>(s_defaultInsertTextImplementation)(responder, methodSelector, textString);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NSResponderMethodHookNotifications::NoResponderForHook(NSResponder* responder,
                                                                SEL methodSelector,
                                                                SEL eventSelector)
    {
        bool handled = false;
        NSResponderMethodHookNotificationBus::BroadcastResult(
            handled,
            &NSResponderMethodHookNotifications::NoResponderFor,
            responder,
            eventSelector);

        if (!handled && s_defaultNoResponderForImplementation)
        {
            reinterpret_cast<cmd_redirector>(s_defaultNoResponderForImplementation)(responder, methodSelector, eventSelector);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NSResponderMethodHookNotifications::DoCommandBySelectorHook(NSResponder* responder,
                                                                     SEL methodSelector,
                                                                     SEL commandSelector)
    {
        bool handled = false;
        NSResponderMethodHookNotificationBus::BroadcastResult(
            handled,
            &NSResponderMethodHookNotifications::DoCommandBySelector,
            responder,
            commandSelector);

        if (!handled && s_defaultDoCommandBySelectorImplementation)
        {
            reinterpret_cast<cmd_redirector>(s_defaultDoCommandBySelectorImplementation)(responder, methodSelector, commandSelector);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Table of key ids indexed by their Mac key code
    const AZStd::array<const InputChannelId*, 128> InputChannelIdByKeyCodeTable =
    {{
        &InputDeviceKeyboard::Key::AlphanumericA,         // 0x00 kVK_ANSI_A
        &InputDeviceKeyboard::Key::AlphanumericS,         // 0x01 kVK_ANSI_S
        &InputDeviceKeyboard::Key::AlphanumericD,         // 0x02 kVK_ANSI_D
        &InputDeviceKeyboard::Key::AlphanumericF,         // 0x03 kVK_ANSI_F
        &InputDeviceKeyboard::Key::AlphanumericH,         // 0x04 kVK_ANSI_H
        &InputDeviceKeyboard::Key::AlphanumericG,         // 0x05 kVK_ANSI_G
        &InputDeviceKeyboard::Key::AlphanumericZ,         // 0x06 kVK_ANSI_Z
        &InputDeviceKeyboard::Key::AlphanumericX,         // 0x07 kVK_ANSI_X
        &InputDeviceKeyboard::Key::AlphanumericC,         // 0x08 kVK_ANSI_C
        &InputDeviceKeyboard::Key::AlphanumericV,         // 0x09 kVK_ANSI_V
        &InputDeviceKeyboard::Key::SupplementaryISO,      // 0x0A kVK_ISO_Section
        &InputDeviceKeyboard::Key::AlphanumericB,         // 0x0B kVK_ANSI_B
        &InputDeviceKeyboard::Key::AlphanumericQ,         // 0x0C kVK_ANSI_Q
        &InputDeviceKeyboard::Key::AlphanumericW,         // 0x0D kVK_ANSI_W
        &InputDeviceKeyboard::Key::AlphanumericE,         // 0x0E kVK_ANSI_E
        &InputDeviceKeyboard::Key::AlphanumericR,         // 0x0F kVK_ANSI_R

        &InputDeviceKeyboard::Key::AlphanumericY,         // 0x10 kVK_ANSI_Y
        &InputDeviceKeyboard::Key::AlphanumericT,         // 0x11 kVK_ANSI_T
        &InputDeviceKeyboard::Key::Alphanumeric1,         // 0x12 kVK_ANSI_1
        &InputDeviceKeyboard::Key::Alphanumeric2,         // 0x13 kVK_ANSI_2
        &InputDeviceKeyboard::Key::Alphanumeric3,         // 0x14 kVK_ANSI_3
        &InputDeviceKeyboard::Key::Alphanumeric4,         // 0x15 kVK_ANSI_4
        &InputDeviceKeyboard::Key::Alphanumeric6,         // 0x16 kVK_ANSI_6
        &InputDeviceKeyboard::Key::Alphanumeric5,         // 0x17 kVK_ANSI_5
        &InputDeviceKeyboard::Key::PunctuationEquals,     // 0x18 kVK_ANSI_Equal
        &InputDeviceKeyboard::Key::Alphanumeric9,         // 0x19 kVK_ANSI_9
        &InputDeviceKeyboard::Key::Alphanumeric7,         // 0x1A kVK_ANSI_7
        &InputDeviceKeyboard::Key::PunctuationHyphen,     // 0x1B kVK_ANSI_Minus
        &InputDeviceKeyboard::Key::Alphanumeric8,         // 0x1C kVK_ANSI_8
        &InputDeviceKeyboard::Key::Alphanumeric0,         // 0x1D kVK_ANSI_0
        &InputDeviceKeyboard::Key::PunctuationBracketR,   // 0x1E kVK_ANSI_RightBracket
        &InputDeviceKeyboard::Key::AlphanumericO,         // 0x1F kVK_ANSI_O

        &InputDeviceKeyboard::Key::AlphanumericU,         // 0x20 kVK_ANSI_U
        &InputDeviceKeyboard::Key::PunctuationBracketL,   // 0x21 kVK_ANSI_LeftBracket
        &InputDeviceKeyboard::Key::AlphanumericI,         // 0x22 kVK_ANSI_I
        &InputDeviceKeyboard::Key::AlphanumericP,         // 0x23 kVK_ANSI_P
        &InputDeviceKeyboard::Key::EditEnter,             // 0x24 kVK_Return
        &InputDeviceKeyboard::Key::AlphanumericL,         // 0x25 kVK_ANSI_L
        &InputDeviceKeyboard::Key::AlphanumericJ,         // 0x26 kVK_ANSI_J
        &InputDeviceKeyboard::Key::PunctuationApostrophe, // 0x27 kVK_ANSI_Quote
        &InputDeviceKeyboard::Key::AlphanumericK,         // 0x28 kVK_ANSI_K
        &InputDeviceKeyboard::Key::PunctuationSemicolon,  // 0x29 kVK_ANSI_Semicolon
        &InputDeviceKeyboard::Key::PunctuationBackslash,  // 0x2A kVK_ANSI_Backslash
        &InputDeviceKeyboard::Key::PunctuationComma,      // 0x2B kVK_ANSI_Comma
        &InputDeviceKeyboard::Key::PunctuationSlash,      // 0x2C kVK_ANSI_Slash
        &InputDeviceKeyboard::Key::AlphanumericN,         // 0x2D kVK_ANSI_N
        &InputDeviceKeyboard::Key::AlphanumericM,         // 0x2E kVK_ANSI_M
        &InputDeviceKeyboard::Key::PunctuationPeriod,     // 0x2F kVK_ANSI_Period

        &InputDeviceKeyboard::Key::EditTab,               // 0x30 kVK_Tab
        &InputDeviceKeyboard::Key::EditSpace,             // 0x31 kVK_Space
        &InputDeviceKeyboard::Key::PunctuationTilde,      // 0x32 kVK_ANSI_Grave
        &InputDeviceKeyboard::Key::EditBackspace,         // 0x33 kVK_Delete
        nullptr,                                          // 0x34 ?
        &InputDeviceKeyboard::Key::Escape,                // 0x35 kVK_Escape
        &InputDeviceKeyboard::Key::ModifierSuperR,        // 0x36 kVK_RightCommand
        &InputDeviceKeyboard::Key::ModifierSuperL,        // 0x37 kVK_Command
        &InputDeviceKeyboard::Key::ModifierShiftL,        // 0x38 kVK_Shift
        &InputDeviceKeyboard::Key::EditCapsLock,          // 0x39 kVK_CapsLock
        &InputDeviceKeyboard::Key::ModifierAltL,          // 0x3A kVK_Option
        &InputDeviceKeyboard::Key::ModifierCtrlL,         // 0x3B kVK_Control
        &InputDeviceKeyboard::Key::ModifierShiftR,        // 0x3C kVK_RightShift
        &InputDeviceKeyboard::Key::ModifierAltR,          // 0x3D kVK_RightOption
        &InputDeviceKeyboard::Key::ModifierCtrlR,         // 0x3E kVK_RightControl
        nullptr,                                          // 0x3F kVK_Function

        &InputDeviceKeyboard::Key::Function17,            // 0x40 kVK_F17
        &InputDeviceKeyboard::Key::NumPadDecimal,         // 0x41 kVK_ANSI_KeypadDecimal
        nullptr,                                          // 0x42 ?
        &InputDeviceKeyboard::Key::NumPadMultiply,        // 0x43 kVK_ANSI_KeypadMultiply
        nullptr,                                          // 0x44 ?
        &InputDeviceKeyboard::Key::NumPadAdd,             // 0x45 kVK_ANSI_KeypadPlus
        nullptr,                                          // 0x46 ?
        &InputDeviceKeyboard::Key::NumLock,               // 0x47 kVK_ANSI_KeypadClear
        nullptr,                                          // 0x48 kVK_VolumeUp
        nullptr,                                          // 0x49 kVK_VolumeDown
        nullptr,                                          // 0x4A kVK_Mute
        &InputDeviceKeyboard::Key::NumPadDivide,          // 0x4B kVK_ANSI_KeypadDivide
        &InputDeviceKeyboard::Key::NumPadEnter,           // 0x4C kVK_ANSI_KeypadEnter
        nullptr,                                          // 0x4D ?
        &InputDeviceKeyboard::Key::NumPadSubtract,        // 0x4E kVK_ANSI_KeypadMinus
        &InputDeviceKeyboard::Key::Function18,            // 0x4F kVK_F18
  
        &InputDeviceKeyboard::Key::Function19,            // 0x50 kVK_F19
        nullptr,                                          // 0x51 kVK_ANSI_KeypadEquals
        &InputDeviceKeyboard::Key::NumPad0,               // 0x52 kVK_ANSI_Keypad0
        &InputDeviceKeyboard::Key::NumPad1,               // 0x53 kVK_ANSI_Keypad1
        &InputDeviceKeyboard::Key::NumPad2,               // 0x54 kVK_ANSI_Keypad2
        &InputDeviceKeyboard::Key::NumPad3,               // 0x55 kVK_ANSI_Keypad3
        &InputDeviceKeyboard::Key::NumPad4,               // 0x56 kVK_ANSI_Keypad4
        &InputDeviceKeyboard::Key::NumPad5,               // 0x57 kVK_ANSI_Keypad5
        &InputDeviceKeyboard::Key::NumPad6,               // 0x58 kVK_ANSI_Keypad6
        &InputDeviceKeyboard::Key::NumPad7,               // 0x59 kVK_ANSI_Keypad7
        &InputDeviceKeyboard::Key::Function20,            // 0x5A kVK_F20
        &InputDeviceKeyboard::Key::NumPad8,               // 0x5B kVK_ANSI_Keypad8
        &InputDeviceKeyboard::Key::NumPad9,               // 0x5C kVK_ANSI_Keypad9
        nullptr,                                          // 0x5D kVK_JIS_Yen
        nullptr,                                          // 0x5E kVK_JIS_Underscore
        nullptr,                                          // 0x5F kVK_JIS_KeypadComma

        &InputDeviceKeyboard::Key::Function05,            // 0x60 kVK_F5
        &InputDeviceKeyboard::Key::Function06,            // 0x61 kVK_F6
        &InputDeviceKeyboard::Key::Function07,            // 0x62 kVK_F7
        &InputDeviceKeyboard::Key::Function03,            // 0x63 kVK_F3
        &InputDeviceKeyboard::Key::Function08,            // 0x64 kVK_F8
        &InputDeviceKeyboard::Key::Function09,            // 0x65 kVK_F9
        nullptr,                                          // 0x66 kVK_JIS_Eisu
        &InputDeviceKeyboard::Key::Function11,            // 0x67 kVK_F11
        nullptr,                                          // 0x68 kVK_JIS_Kana
        &InputDeviceKeyboard::Key::Function13,            // 0x69 kVK_F13
        &InputDeviceKeyboard::Key::Function16,            // 0x6A kVK_F16
        &InputDeviceKeyboard::Key::Function14,            // 0x6B kVK_F14
        nullptr,                                          // 0x6C ?
        &InputDeviceKeyboard::Key::Function10,            // 0x6D kVK_F10
        nullptr,                                          // 0x6E ?
        &InputDeviceKeyboard::Key::Function12,            // 0x6F kVK_F12

        nullptr,                                          // 0x70 ?
        &InputDeviceKeyboard::Key::Function15,            // 0x71 kVK_F15
        nullptr,                                          // 0x72 kVK_Help
        &InputDeviceKeyboard::Key::NavigationHome,        // 0x73 kVK_Home
        &InputDeviceKeyboard::Key::NavigationPageUp,      // 0x74 kVK_PageUp
        &InputDeviceKeyboard::Key::NavigationDelete,      // 0x75 kVK_ForwardDelete
        &InputDeviceKeyboard::Key::Function04,            // 0x76 kVK_F4
        &InputDeviceKeyboard::Key::NavigationEnd,         // 0x77 kVK_End
        &InputDeviceKeyboard::Key::Function02,            // 0x78 kVK_F2
        &InputDeviceKeyboard::Key::NavigationPageDown,    // 0x79 kVK_PageDown
        &InputDeviceKeyboard::Key::Function01,            // 0x7A kVK_F1
        &InputDeviceKeyboard::Key::NavigationArrowLeft,   // 0x7B kVK_LeftArrow
        &InputDeviceKeyboard::Key::NavigationArrowRight,  // 0x7C kVK_RightArrow
        &InputDeviceKeyboard::Key::NavigationArrowDown,   // 0x7D kVK_DownArrow
        &InputDeviceKeyboard::Key::NavigationArrowUp,     // 0x7E kVK_UpArrow
        nullptr                                           // 0x7F ?
    }};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // NSEventType enum constant names were changed in macOS 10.12, but our min-spec is still 10.10
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 101200 // __MAC_10_12 may not be defined by all earlier sdks
    static const NSEventType NSEventTypeKeyDown         = NSKeyDown;
    static const NSEventType NSEventTypeKeyUp           = NSKeyUp;
    static const NSEventType NSEventTypeFlagsChanged    = NSFlagsChanged;

    // kVK_RightCommand was also added in macOS 10.12
    static const int kVK_RightCommand                   = 0x36;
#endif // __MAC_OS_X_VERSION_MAX_ALLOWED < 101200
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for Mac keyboard input devices
    class InputDeviceKeyboardMac : public InputDeviceKeyboard::Implementation
                                 , public RawInputNotificationBusMac::Handler
                                 , public NSResponderMethodHookNotificationBus::Handler
    {
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Count of the number instances of this class that have been created
        static int s_instanceCount;

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceKeyboardMac, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceKeyboardMac(InputDeviceKeyboard& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceKeyboardMac() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceKeyboard::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceKeyboard::Implementation::HasTextEntryStarted
        bool HasTextEntryStarted() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceKeyboard::Implementation::TextEntryStart
        void TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions& options) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceKeyboard::Implementation::TextEntryStop
        void TextEntryStop() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceKeyboard::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsMac::OnRawInputEvent
        void OnRawInputEvent(const NSEvent* nsEvent) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // \ref NSResponderMethodHookNotifications::InsertText
        bool InsertText(NSResponder* responder, id textString) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // \ref NSResponderMethodHookNotifications::NoResponderFor
        bool NoResponderFor(NSResponder* responder, SEL eventSelector) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // \ref NSResponderMethodHookNotifications::DoCommandBySelector
        bool DoCommandBySelector(NSResponder* responder, SEL commandSelector) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to queue standard key events processed in OnRawInputEvent
        //! \param[in] keyCode The Mac specific key code
        //! \param[in] keyState The key state (down or up)
        void QueueRawStandardKeyEvent(AZ::u32 keyCode, bool keyState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to queue modifier key events processed in OnRawInputEvent
        //! \param[in] keyCode The Mac specific key code
        //! \param[in] modifierFlags The event's modifier flags
        void QueueRawModifierKeyEvent(AZ::u32 keyCode, AZ::u32 modifierFlags);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! A dummy NSView used to interpret key down events into text
        NSView* m_textInterpreterView = nullptr;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Has text entry been started?
        bool m_hasTextEntryStarted = false;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Does the application's main window currently have focus?
        bool m_hasFocus = false;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    int InputDeviceKeyboardMac::s_instanceCount = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboardMac::InputDeviceKeyboardMac(InputDeviceKeyboard& inputDevice)
        : InputDeviceKeyboard::Implementation(inputDevice)
        , m_textInterpreterView(nullptr)
        , m_hasTextEntryStarted(false)
        , m_hasFocus(false)
    {
        if (s_instanceCount++ == 0)
        {
            NSResponderMethodHookNotifications::InsertHooks();
        }

        // Create an NSView that we can call interpretKeyEvents on to process text input.
        m_textInterpreterView = [[NSView alloc] initWithFrame: CGRectZero];

        RawInputNotificationBusMac::Handler::BusConnect();
        NSResponderMethodHookNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboardMac::~InputDeviceKeyboardMac()
    {
        NSResponderMethodHookNotificationBus::Handler::BusDisconnect();
        RawInputNotificationBusMac::Handler::BusDisconnect();

        if (m_textInterpreterView)
        {
            [m_textInterpreterView release];
            m_textInterpreterView = nullptr;
        }

        if (--s_instanceCount == 0)
        {
            NSResponderMethodHookNotifications::RemoveHooks();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboardMac::IsConnected() const
    {
        // If necessary we may be able to determine the connected state using the I/O Kit HIDManager:
        // https://developer.apple.com/library/content/documentation/DeviceDrivers/Conceptual/HID/new_api_10_5/tn2187.html
        //
        // Doing this may allow (and perhaps even force) us to distinguish between multiple physical
        // devices of the same type. But given support for multiple keyboards is a fairly niche need
        // we'll keep things simple (for now) and assume there's one (and only 1) keyboard connected
        // at all times. In practice this means if multiple physical keyboards are connected we will
        // process input from them all, but treat all the input as if it comes from the same device.
        //
        // If it becomes necessary to determine connected states of keyboard devices (and/or support
        // distinguishing between multiple physical keyboards) we should implement this function and
        // call BroadcastInputDeviceConnectedEvent/BroadcastInputDeviceDisconnectedEvent when needed.
        //
        // Note that doing so will require modifying how we create and manage keyboard input devices
        // in InputSystemComponent/InputSystemComponentWin so we create multiple InputDeviceKeyboard
        // instances (somehow associating each with a raw input device id), along with modifying the
        // InputDeviceKeyboardMac::OnRawInputEvent function to filter incoming events by this raw id.
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboardMac::HasTextEntryStarted() const
    {
        return m_hasTextEntryStarted;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardMac::TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions&)
    {
        m_hasTextEntryStarted = true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardMac::TextEntryStop()
    {
        m_hasTextEntryStarted = false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardMac::TickInputDevice()
    {
        // The event loop has just been pumped in ApplicationRequests::PumpSystemEventLoopUntilEmpty,
        // so we now just need to process any raw events that have been queued since the last frame
        const bool hadFocus = m_hasFocus;
        m_hasFocus = NSApplication.sharedApplication.active;
        if (m_hasFocus)
        {
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
    void InputDeviceKeyboardMac::OnRawInputEvent(const NSEvent* nsEvent)
    {
        if (!NSApplication.sharedApplication.active)
        {
            return;
        }

        switch (nsEvent.type)
        {
            case NSEventTypeKeyDown:
            {
                // We can ignore repeat events here...
                if (!nsEvent.isARepeat)
                {
                    QueueRawStandardKeyEvent(nsEvent.keyCode, true);
                }

                // ...but they should still generate text.
            #if !defined(ALWAYS_DISPATCH_KEYBOARD_TEXT_INPUT)
                if (m_hasTextEntryStarted)
            #endif // defined(ALWAYS_DISPATCH_KEYBOARD_TEXT_INPUT)
                {
                    // This is important, otherwise interpretKeyEvents may do nothing
                    [NSApplication.sharedApplication.mainWindow makeFirstResponder: nil];

                    // Translate key presses into text that will get sent on
                    // to NSResponderMethodHookNotifications::InsertTextHook
                    [m_textInterpreterView interpretKeyEvents: [NSArray arrayWithObject: nsEvent]];

                    if (nsEvent.keyCode == kVK_Delete)
                    {
                        // Emulate Windows where the backspace key generates a '\b' character
                        const AZStd::string textUTF8 = "\b";
                        QueueRawTextEvent(textUTF8);
                    }
                }
            }
            break;
            case NSEventTypeKeyUp:
            {
                QueueRawStandardKeyEvent(nsEvent.keyCode, false);
            }
            break;
            case NSEventTypeFlagsChanged:
            {
                QueueRawModifierKeyEvent(nsEvent.keyCode, static_cast<AZ::u32>(nsEvent.modifierFlags));
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
    bool InputDeviceKeyboardMac::InsertText(NSResponder* responder, id textString)
    {
        if (responder != m_textInterpreterView)
        {
            // We don't own the responder that this method was invoked on
            return false;
        }

        if (!NSApplication.sharedApplication.active)
        {
            // This application is not active
            return false;
        }

        const bool isAttributed = [textString isKindOfClass: [NSAttributedString class]];
        const AZStd::string textUTF8 = isAttributed ?
                                       [textString string].UTF8String :
                                       [textString UTF8String];
        QueueRawTextEvent(textUTF8);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboardMac::NoResponderFor(NSResponder* /*responder*/, SEL /*eventSelector*/)
    {
        // Do nothing, but return true so we don't invoke the default behavior that calls NSBeep.
        // This method is only ever called on the main NSWindow object, so while it is not ideal
        // that we're intercepting methods to an object we don't own it's the only way to ensure
        // (in all circumstances) that a beeping sound isn't emitted when pressing keyboard keys.
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboardMac::DoCommandBySelector(NSResponder* responder, SEL /*commandSelector*/)
    {
        // If we own the responder that this method was invoked on return
        // true so we don't invoke the default behavior that calls NSBeep
        return responder == m_textInterpreterView;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardMac::QueueRawStandardKeyEvent(AZ::u32 keyCode, bool keyState)
    {
        if ((keyCode == kVK_ISO_Section || keyCode == kVK_ANSI_Grave) &&
            KBGetLayoutType(LMGetKbdType()) == kKeyboardISO)
        {
            // Mac swaps these two key codes for keyboards that use an ISO mechanical layout,
            // so we have to swap them back.
            keyCode = (kVK_ISO_Section + kVK_ANSI_Grave) - keyCode;
        }

        const InputChannelId* channelId = (keyCode < InputChannelIdByKeyCodeTable.size()) ?
                                          InputChannelIdByKeyCodeTable[keyCode] : nullptr;
        if (channelId)
        {
            QueueRawKeyEvent(*channelId, keyState);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardMac::QueueRawModifierKeyEvent(AZ::u32 keyCode, AZ::u32 modifierFlags)
    {
        const InputChannelId* channelId = (keyCode < InputChannelIdByKeyCodeTable.size()) ?
                                          InputChannelIdByKeyCodeTable[keyCode] : nullptr;
        if (!channelId)
        {
            return;
        }

        switch (keyCode)
        {
            case kVK_Option:
            {
                QueueRawKeyEvent(*channelId, modifierFlags & NX_DEVICELALTKEYMASK);
            }
            break;
            case kVK_RightOption:
            {
                QueueRawKeyEvent(*channelId, modifierFlags & NX_DEVICERALTKEYMASK);
            }
            break;
            case kVK_Control:
            {
                QueueRawKeyEvent(*channelId, modifierFlags & NX_DEVICELCTLKEYMASK);
            }
            break;
            case kVK_RightControl:
            {
                QueueRawKeyEvent(*channelId, modifierFlags & NX_DEVICERCTLKEYMASK);
            }
            break;
            case kVK_Shift:
            {
                QueueRawKeyEvent(*channelId, modifierFlags & NX_DEVICELSHIFTKEYMASK);
            }
            break;
            case kVK_RightShift:
            {
                QueueRawKeyEvent(*channelId, modifierFlags & NX_DEVICERSHIFTKEYMASK);
            }
            break;
            case kVK_Command:
            {
                QueueRawKeyEvent(*channelId, modifierFlags & NX_DEVICELCMDKEYMASK);
            }
            break;
            case kVK_RightCommand:
            {
                QueueRawKeyEvent(*channelId, modifierFlags & NX_DEVICERCMDKEYMASK);
            }
            break;
            case kVK_CapsLock:
            {
                // Caps lock is annoying in that it only reports events on key up (when the state of
                // the key changes), and never on key down, making it unlike all other keyboard keys.
                // While not ideal, simply sending both 'down' and 'up' events in succession when we
                // detect a change to the caps lock modifier works well enough, although it means we
                // will never be able to detect if the caps lock key is being held down.
                QueueRawKeyEvent(*channelId, true);
                QueueRawKeyEvent(*channelId, false);
            }
            break;
            default:
            {
                // Not a supported modifier key
            }
            break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class MacDeviceKeyboardImplFactory
        : public InputDeviceKeyboard::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceKeyboard::Implementation> Create(InputDeviceKeyboard& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceKeyboardMac>(inputDevice);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeDeviceKeyboardImplementationFactory()
    {
        m_deviceKeyboardImplFactory = AZStd::make_unique<MacDeviceKeyboardImplFactory>();
        AZ::Interface<InputDeviceKeyboard::ImplementationFactory>::Register(m_deviceKeyboardImplFactory.get());
    }
} // namespace AzFramework
