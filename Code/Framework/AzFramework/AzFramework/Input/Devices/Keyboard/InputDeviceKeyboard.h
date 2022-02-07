/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Requests/InputTextEntryRequestBus.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedModifierKeyStates.h>
#include <AzFramework/Input/Devices/InputDevice.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    // Ideally, we would only dispatch text input while it has been explicitly enabled by a call to
    // TextEntryStart (paired with a call to TextEntryStop), but to maintain compatibility with the
    // existing behavior we must always dispatch keyboard text input by default. Remove this define
    // if you want to control text input event dispatch using TextEntryStart and TextEntryStop.
    #define ALWAYS_DISPATCH_KEYBOARD_TEXT_INPUT

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Defines a generic keyboard input device, including the ids of all associated input channels.
    //! Platform specific implementations are defined as private implementations so that creating an
    //! instance of this generic class will work correctly on any platform supporting keyboard input,
    //! while providing access to the device name and associated channel ids on any platform through
    //! the 'null' implementation (primarily so that the editor can use them to setup input mappings).
    class InputDeviceKeyboard : public InputDevice
                              , public InputTextEntryRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The id used to identify the primary physical keyboard input device
        static const InputDeviceId Id;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input device id identifies a physical keyboard (regardless of index)
        //! \param[in] inputDeviceId The input device id to check
        //! \return True if the input device id identifies a physical keyboard, false otherwise
        static bool IsKeyboardDevice(const InputDeviceId& inputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify standard physical keyboard keys intended for use
        //! as gameplay controls (not virtual keys or ascii/unicode keycodes). They are grouped into
        //! categories (roughly based on their physical location and their standard use) as follows.
        //!
        //! Note that all these key ids correspond to the physical keys of an ANSI mechanical layout
        //! as marked using the standard QWERTY visual layout, except for the ISOAdditional id which
        //! corresponds to the additional key (next to left-shift) present on ISO mechanical layouts.
        //! The additional keys found on keyboards that use JIS mechanical layouts are not supported.
        //!
        //! Alphanumeric Keys
        //! - The A-Z and 0-9 keys
        //! - Present on almost all types of physical keyboards
        //!
        //! Edit (and escape) Keys
        //! - The backspace, caps lock, enter, space, tab, and escape keys
        //! - Present on almost all types of physical keyboards
        //!
        //! Function Keys
        //! - The F1-F12 keys are present on almost all types of physical keyboards
        //! - The F13-F20 keys are only present on some physical keyboards, so their use
        //!   should be avoided if you wish to supprt the widest range of keyboard devices
        //!
        //! Modifier Keys
        //! - The (left and right) alt, control, shift, and 'super' (windows/apple) keys
        //! - Present on almost all types of physical keyboards
        //!
        //! Navigation Keys
        //! - The arrow, delete, insert, home, end, and page up/down keys
        //! - Not always present on smaller (eg. laptop) keyboards
        //! - Their use should be avoided if you wish to supprt the widest range of keyboard devices
        //!
        //! Numpad Keys
        //! - The various number pad (or keypad) keys, including num lock
        //! - Not always present on smaller (eg. laptop) keyboards
        //! - These ids will be used regardless of whether the num lock key is active
        //! - Their use should be avoided if you wish to supprt the widest range of keyboard devices
        //!
        //! Punctuation Keys
        //! - The various punctuation character keys (eg. comma, period, slash)
        //! - Present on almost all types of physical keyboards
        //! - Not generally used as input for games
        //!
        //! Supplementary ISO Key
        //! - The additional key (found to the right of the left-shift key) on ISO keyboards
        //! - Its use should be avoided if you wish to support the widest range of keyboard devices
        //!
        //! Windows System Keys
        //! - The windows specific pause/break, print/sysrq, and scroll lock keys
        //! - Their use should be avoided if you wish to supprt the widest range of keyboard devices
        struct Key
        {
            // Alphanumeric Keys
            static constexpr inline InputChannelId Alphanumeric0{"keyboard_key_alphanumeric_0"}; //!< The 0 key
            static constexpr inline InputChannelId Alphanumeric1{"keyboard_key_alphanumeric_1"}; //!< The 1 key
            static constexpr inline InputChannelId Alphanumeric2{"keyboard_key_alphanumeric_2"}; //!< The 2 key
            static constexpr inline InputChannelId Alphanumeric3{"keyboard_key_alphanumeric_3"}; //!< The 3 key
            static constexpr inline InputChannelId Alphanumeric4{"keyboard_key_alphanumeric_4"}; //!< The 4 key
            static constexpr inline InputChannelId Alphanumeric5{"keyboard_key_alphanumeric_5"}; //!< The 5 key
            static constexpr inline InputChannelId Alphanumeric6{"keyboard_key_alphanumeric_6"}; //!< The 6 key
            static constexpr inline InputChannelId Alphanumeric7{"keyboard_key_alphanumeric_7"}; //!< The 7 key
            static constexpr inline InputChannelId Alphanumeric8{"keyboard_key_alphanumeric_8"}; //!< The 8 key
            static constexpr inline InputChannelId Alphanumeric9{"keyboard_key_alphanumeric_9"}; //!< The 9 key
            static constexpr inline InputChannelId AlphanumericA{"keyboard_key_alphanumeric_A"}; //!< The A key
            static constexpr inline InputChannelId AlphanumericB{"keyboard_key_alphanumeric_B"}; //!< The B key
            static constexpr inline InputChannelId AlphanumericC{"keyboard_key_alphanumeric_C"}; //!< The C key
            static constexpr inline InputChannelId AlphanumericD{"keyboard_key_alphanumeric_D"}; //!< The D key
            static constexpr inline InputChannelId AlphanumericE{"keyboard_key_alphanumeric_E"}; //!< The E key
            static constexpr inline InputChannelId AlphanumericF{"keyboard_key_alphanumeric_F"}; //!< The F key
            static constexpr inline InputChannelId AlphanumericG{"keyboard_key_alphanumeric_G"}; //!< The G key
            static constexpr inline InputChannelId AlphanumericH{"keyboard_key_alphanumeric_H"}; //!< The H key
            static constexpr inline InputChannelId AlphanumericI{"keyboard_key_alphanumeric_I"}; //!< The I key
            static constexpr inline InputChannelId AlphanumericJ{"keyboard_key_alphanumeric_J"}; //!< The J key
            static constexpr inline InputChannelId AlphanumericK{"keyboard_key_alphanumeric_K"}; //!< The K key
            static constexpr inline InputChannelId AlphanumericL{"keyboard_key_alphanumeric_L"}; //!< The L key
            static constexpr inline InputChannelId AlphanumericM{"keyboard_key_alphanumeric_M"}; //!< The M key
            static constexpr inline InputChannelId AlphanumericN{"keyboard_key_alphanumeric_N"}; //!< The N key
            static constexpr inline InputChannelId AlphanumericO{"keyboard_key_alphanumeric_O"}; //!< The O key
            static constexpr inline InputChannelId AlphanumericP{"keyboard_key_alphanumeric_P"}; //!< The P key
            static constexpr inline InputChannelId AlphanumericQ{"keyboard_key_alphanumeric_Q"}; //!< The Q key
            static constexpr inline InputChannelId AlphanumericR{"keyboard_key_alphanumeric_R"}; //!< The R key
            static constexpr inline InputChannelId AlphanumericS{"keyboard_key_alphanumeric_S"}; //!< The S key
            static constexpr inline InputChannelId AlphanumericT{"keyboard_key_alphanumeric_T"}; //!< The T key
            static constexpr inline InputChannelId AlphanumericU{"keyboard_key_alphanumeric_U"}; //!< The U key
            static constexpr inline InputChannelId AlphanumericV{"keyboard_key_alphanumeric_V"}; //!< The V key
            static constexpr inline InputChannelId AlphanumericW{"keyboard_key_alphanumeric_W"}; //!< The W key
            static constexpr inline InputChannelId AlphanumericX{"keyboard_key_alphanumeric_X"}; //!< The X key
            static constexpr inline InputChannelId AlphanumericY{"keyboard_key_alphanumeric_Y"}; //!< The Y key
            static constexpr inline InputChannelId AlphanumericZ{"keyboard_key_alphanumeric_Z"}; //!< The Z key

            // Edit {and escape} Keys
            static constexpr inline InputChannelId EditBackspace{"keyboard_key_edit_backspace"}; //!< The backspace key
            static constexpr inline InputChannelId EditCapsLock{"keyboard_key_edit_capslock"}; //!< The caps lock key
            static constexpr inline InputChannelId EditEnter{"keyboard_key_edit_enter"}; //!< The enter/return key
            static constexpr inline InputChannelId EditSpace{"keyboard_key_edit_space"}; //!< The spacebar key
            static constexpr inline InputChannelId EditTab{"keyboard_key_edit_tab"}; //!< The tab key
            static constexpr inline InputChannelId Escape{"keyboard_key_escape"}; //!< The escape key

            // Function Keys
            static constexpr inline InputChannelId Function01{"keyboard_key_function_F01"}; //!< The F1 key
            static constexpr inline InputChannelId Function02{"keyboard_key_function_F02"}; //!< The F2 key
            static constexpr inline InputChannelId Function03{"keyboard_key_function_F03"}; //!< The F3 key
            static constexpr inline InputChannelId Function04{"keyboard_key_function_F04"}; //!< The F4 key
            static constexpr inline InputChannelId Function05{"keyboard_key_function_F05"}; //!< The F5 key
            static constexpr inline InputChannelId Function06{"keyboard_key_function_F06"}; //!< The F6 key
            static constexpr inline InputChannelId Function07{"keyboard_key_function_F07"}; //!< The F7 key
            static constexpr inline InputChannelId Function08{"keyboard_key_function_F08"}; //!< The F8 key
            static constexpr inline InputChannelId Function09{"keyboard_key_function_F09"}; //!< The F9 key
            static constexpr inline InputChannelId Function10{"keyboard_key_function_F10"}; //!< The F10 key
            static constexpr inline InputChannelId Function11{"keyboard_key_function_F11"}; //!< The F11 key
            static constexpr inline InputChannelId Function12{"keyboard_key_function_F12"}; //!< The F12 key
            static constexpr inline InputChannelId Function13{"keyboard_key_function_F13"}; //!< The F13 key
            static constexpr inline InputChannelId Function14{"keyboard_key_function_F14"}; //!< The F14 key
            static constexpr inline InputChannelId Function15{"keyboard_key_function_F15"}; //!< The F15 key
            static constexpr inline InputChannelId Function16{"keyboard_key_function_F16"}; //!< The F16 key
            static constexpr inline InputChannelId Function17{"keyboard_key_function_F17"}; //!< The F17 key
            static constexpr inline InputChannelId Function18{"keyboard_key_function_F18"}; //!< The F18 key
            static constexpr inline InputChannelId Function19{"keyboard_key_function_F19"}; //!< The F19 key
            static constexpr inline InputChannelId Function20{"keyboard_key_function_F20"}; //!< The F20 key

            // Modifier Keys
            static constexpr inline InputChannelId ModifierAltL{"keyboard_key_modifier_alt_l"}; //!< The left alt/option key
            static constexpr inline InputChannelId ModifierAltR{"keyboard_key_modifier_alt_r"}; //!< The right alt/option key
            static constexpr inline InputChannelId ModifierCtrlL{"keyboard_key_modifier_ctrl_l"}; //!< The left control key
            static constexpr inline InputChannelId ModifierCtrlR{"keyboard_key_modifier_ctrl_r"}; //!< The right control key
            static constexpr inline InputChannelId ModifierShiftL{"keyboard_key_modifier_shift_l"}; //!< The left shift key
            static constexpr inline InputChannelId ModifierShiftR{"keyboard_key_modifier_shift_r"}; //!< The right shift key
            static constexpr inline InputChannelId ModifierSuperL{"keyboard_key_modifier_super_l"}; //!< The left super {windows or apple} key
            static constexpr inline InputChannelId ModifierSuperR{"keyboard_key_modifier_super_r"}; //!< The right super {windows or apple} key

            // Navigation Keys
            static constexpr inline InputChannelId NavigationArrowDown{"keyboard_key_navigation_arrow_down"}; //!< The down arrow key
            static constexpr inline InputChannelId NavigationArrowLeft{"keyboard_key_navigation_arrow_left"}; //!< The left arrow key
            static constexpr inline InputChannelId NavigationArrowRight{"keyboard_key_navigation_arrow_right"}; //!< The right arrow key
            static constexpr inline InputChannelId NavigationArrowUp{"keyboard_key_navigation_arrow_up"}; //!< The up arrow key
            static constexpr inline InputChannelId NavigationDelete{"keyboard_key_navigation_delete"}; //!< The delete key
            static constexpr inline InputChannelId NavigationEnd{"keyboard_key_navigation_end"}; //!< The end key
            static constexpr inline InputChannelId NavigationHome{"keyboard_key_navigation_home"}; //!< The home key
            static constexpr inline InputChannelId NavigationInsert{"keyboard_key_navigation_insert"}; //!< The insert key
            static constexpr inline InputChannelId NavigationPageDown{"keyboard_key_navigation_page_down"}; //!< The page down key
            static constexpr inline InputChannelId NavigationPageUp{"keyboard_key_navigation_page_up"}; //!< The page up key

            // Numpad Keys
            static constexpr inline InputChannelId NumLock{"keyboard_key_num_lock"}; //!< The num lock key {the clear key on apple keyboards}
            static constexpr inline InputChannelId NumPad0{"keyboard_key_numpad_0"}; //!< The numpad 0 key
            static constexpr inline InputChannelId NumPad1{"keyboard_key_numpad_1"}; //!< The numpad 1 key
            static constexpr inline InputChannelId NumPad2{"keyboard_key_numpad_2"}; //!< The numpad 2 key
            static constexpr inline InputChannelId NumPad3{"keyboard_key_numpad_3"}; //!< The numpad 3 key
            static constexpr inline InputChannelId NumPad4{"keyboard_key_numpad_4"}; //!< The numpad 4 key
            static constexpr inline InputChannelId NumPad5{"keyboard_key_numpad_5"}; //!< The numpad 5 key
            static constexpr inline InputChannelId NumPad6{"keyboard_key_numpad_6"}; //!< The numpad 6 key
            static constexpr inline InputChannelId NumPad7{"keyboard_key_numpad_7"}; //!< The numpad 7 key
            static constexpr inline InputChannelId NumPad8{"keyboard_key_numpad_8"}; //!< The numpad 8 key
            static constexpr inline InputChannelId NumPad9{"keyboard_key_numpad_9"}; //!< The numpad 9 key
            static constexpr inline InputChannelId NumPadAdd{"keyboard_key_numpad_add"}; //!< The numpad add key
            static constexpr inline InputChannelId NumPadDecimal{"keyboard_key_numpad_decimal"}; //!< The numpad decimal key
            static constexpr inline InputChannelId NumPadDivide{"keyboard_key_numpad_divide"}; //!< The numpad divide key
            static constexpr inline InputChannelId NumPadEnter{"keyboard_key_numpad_enter"}; //!< The numpad enter key
            static constexpr inline InputChannelId NumPadMultiply{"keyboard_key_numpad_multiply"}; //!< The numpad multiply key
            static constexpr inline InputChannelId NumPadSubtract{"keyboard_key_numpad_subtract"}; //!< The numpad subtract key

            // Punctuation Keys
            static constexpr inline InputChannelId PunctuationApostrophe{"keyboard_key_punctuation_apostrophe"}; //!< The apostrophe key
            static constexpr inline InputChannelId PunctuationBackslash{"keyboard_key_punctuation_backslash"}; //!< The backslash key
            static constexpr inline InputChannelId PunctuationBracketL{"keyboard_key_punctuation_bracket_l"}; //!< The left bracket key
            static constexpr inline InputChannelId PunctuationBracketR{"keyboard_key_punctuation_bracket_r"}; //!< The right bracket key
            static constexpr inline InputChannelId PunctuationComma{"keyboard_key_punctuation_comma"}; //!< The comma key
            static constexpr inline InputChannelId PunctuationEquals{"keyboard_key_punctuation_equals"}; //!< The equals key
            static constexpr inline InputChannelId PunctuationHyphen{"keyboard_key_punctuation_hyphen"}; //!< The hyphen/underscore key
            static constexpr inline InputChannelId PunctuationPeriod{"keyboard_key_punctuation_period"}; //!< The period key
            static constexpr inline InputChannelId PunctuationSemicolon{"keyboard_key_punctuation_semicolon"}; //!< The semicolon key
            static constexpr inline InputChannelId PunctuationSlash{"keyboard_key_punctuation_slash"}; //!< The {forward} slash key
            static constexpr inline InputChannelId PunctuationTilde{"keyboard_key_punctuation_tilde"}; //!< The tilde/grave key

            // Supplementary ISO Key
            static constexpr inline InputChannelId SupplementaryISO{"keyboard_key_supplementary_iso"}; //!< The supplementary ISO layout key

            // Windows System Keys
            static constexpr inline InputChannelId WindowsSystemPause{"keyboard_key_windows_system_pause"}; //!< The windows pause key
            static constexpr inline InputChannelId WindowsSystemPrint{"keyboard_key_windows_system_print"}; //!< The windows print key
            static constexpr inline InputChannelId WindowsSystemScrollLock{"keyboard_key_windows_system_scroll_lock"}; //!< The windows scroll lock key

            //!< All keyboard key ids
            static constexpr inline AZStd::array All
            {
                // Alphanumeric Keys
                Alphanumeric0,
                Alphanumeric1,
                Alphanumeric2,
                Alphanumeric3,
                Alphanumeric4,
                Alphanumeric5,
                Alphanumeric6,
                Alphanumeric7,
                Alphanumeric8,
                Alphanumeric9,
                AlphanumericA,
                AlphanumericB,
                AlphanumericC,
                AlphanumericD,
                AlphanumericE,
                AlphanumericF,
                AlphanumericG,
                AlphanumericH,
                AlphanumericI,
                AlphanumericJ,
                AlphanumericK,
                AlphanumericL,
                AlphanumericM,
                AlphanumericN,
                AlphanumericO,
                AlphanumericP,
                AlphanumericQ,
                AlphanumericR,
                AlphanumericS,
                AlphanumericT,
                AlphanumericU,
                AlphanumericV,
                AlphanumericW,
                AlphanumericX,
                AlphanumericY,
                AlphanumericZ,

                // Edit (and escape) Keys
                EditBackspace,
                EditCapsLock,
                EditEnter,
                EditSpace,
                EditTab,
                Escape,

                // Function Keys
                Function01,
                Function02,
                Function03,
                Function04,
                Function05,
                Function06,
                Function07,
                Function08,
                Function09,
                Function10,
                Function11,
                Function12,
                Function13,
                Function14,
                Function15,
                Function16,
                Function17,
                Function18,
                Function19,
                Function20,

                // Modifier Keys
                ModifierAltL,
                ModifierAltR,
                ModifierCtrlL,
                ModifierCtrlR,
                ModifierShiftL,
                ModifierShiftR,
                ModifierSuperL,
                ModifierSuperR,

                // Navigation Keys
                NavigationArrowDown,
                NavigationArrowLeft,
                NavigationArrowRight,
                NavigationArrowUp,
                NavigationDelete,
                NavigationEnd,
                NavigationHome,
                NavigationInsert,
                NavigationPageDown,
                NavigationPageUp,

                // Numpad Keys
                NumLock,
                NumPad0,
                NumPad1,
                NumPad2,
                NumPad3,
                NumPad4,
                NumPad5,
                NumPad6,
                NumPad7,
                NumPad8,
                NumPad9,
                NumPadAdd,
                NumPadDecimal,
                NumPadDivide,
                NumPadEnter,
                NumPadMultiply,
                NumPadSubtract,

                // Punctuation Keys
                PunctuationApostrophe,
                PunctuationBackslash,
                PunctuationBracketL,
                PunctuationBracketR,
                PunctuationComma,
                PunctuationEquals,
                PunctuationHyphen,
                PunctuationPeriod,
                PunctuationSemicolon,
                PunctuationSlash,
                PunctuationTilde,

                // Supplementary ISO Key
                SupplementaryISO,

                // Windows System Keys
                WindowsSystemPause,
                WindowsSystemPrint,
                WindowsSystemScrollLock
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceKeyboard, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputDeviceKeyboard, "{CFD40F74-81DF-40B1-995B-F7142E6B1259}", InputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        InputDeviceKeyboard(AzFramework::InputDeviceId id = Id);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputDeviceKeyboard);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceKeyboard() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        LocalUserId GetAssignedLocalUserId() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::GetInputChannelsById
        const InputChannelByIdMap& GetInputChannelsById() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::IsSupported
        bool IsSupported() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputTextEntryRequests::HasTextEntryStarted
        bool HasTextEntryStarted() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputTextEntryRequests::TextEntryStart
        void TextEntryStart(const VirtualKeyboardOptions& options) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputTextEntryRequests::TextEntryStop
        void TextEntryStop() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::GetPhysicalKeyOrButtonText
        void GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                        AZStd::string& o_keyOrButtonText) const override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for verbose container class
        using KeyChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelDigitalWithSharedModifierKeyStates*>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        SharedModifierKeyStates m_modifierKeyStates; //!< Shared modifier key states
        InputChannelByIdMap     m_allChannelsById;   //!< All keyboard channels by id
        KeyChannelByIdMap       m_keyChannelsById;   //!< All keyboard key channels by id

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base class for platform specific implementations of keyboard input devices
        class Implementation
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator, 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default factory create function
            //! \param[in] inputDevice Reference to the input device being implemented
            static Implementation* Create(InputDeviceKeyboard& inputDevice);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] inputDevice Reference to the input device being implemented
            Implementation(InputDeviceKeyboard& inputDevice);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Disable copying
            AZ_DISABLE_COPY_MOVE(Implementation);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Destructor
            virtual ~Implementation();

            ////////////////////////////////////////////////////////////////////////////////////////
            virtual LocalUserId GetAssignedLocalUserId() const;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query the connected state of the input device
            //! \return True if the input device is currently connected, false otherwise
            virtual bool IsConnected() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query whether text entry has already been started
            //! \return True if text entry has already been started, false otherwise
            virtual bool HasTextEntryStarted() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Inform input device that text input is expected to start (pair with StopTextInput)
            //! \param[in] options Used to specify the appearance/behavior of any virtual keyboard
            virtual void TextEntryStart(const VirtualKeyboardOptions& options) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Inform input device that text input is expected to stop (pair with StartTextInput)
            virtual void TextEntryStop() = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Tick/update the input device to broadcast all input events since the last frame
            virtual void TickInputDevice() = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Get the text displayed on the physical key/button associated with an input channel.
            //! In the case of keyboard keys, we must take into account the current keyboard layout.
            //! \param[in] inputChannelId The input channel id whose key or button text to return
            //! \param[out] o_keyOrButtonText The text displayed on the physical key/button if found
            virtual void GetPhysicalKeyOrButtonText(const InputChannelId& /*inputChannelId*/,
                                                    AZStd::string& /*o_keyOrButtonText*/) const {}

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Queue raw key events to be processed in the next call to ProcessRawEventQueues.
            //! This function is not thread safe and so should only be called from the main thread.
            //! \param[in] inputChannelId The input channel id
            //! \param[in] rawKeyState The raw key state
            void QueueRawKeyEvent(const InputChannelId& inputChannelId, bool rawKeyState);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Queue raw text events to be processed in the next call to ProcessRawEventQueues.
            //! This function is not thread safe and so should only be called from the main thread.
            //! \param[in] textUTF8 The text to queue (encoded using UTF-8)
            void QueueRawTextEvent(const AZStd::string& textUTF8);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process raw input events that have been queued since the last call to this function.
            //! This function is not thread safe, and so should only be called from the main thread.
            void ProcessRawEventQueues();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Reset the state of all this input device's associated input channels
            void ResetInputChannelStates();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Alias for verbose container class
            using RawKeyEventQueueByIdMap = AZStd::unordered_map<InputChannelId, AZStd::vector<bool>>;

            ////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            InputDeviceKeyboard&         m_inputDevice;           //!< Reference to the input device
            RawKeyEventQueueByIdMap      m_rawKeyEventQueuesById; //!< Raw key event queues by id
            AZStd::vector<AZStd::string> m_rawTextEventQueue;     //!< Raw text event queue
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the implementation of this input device
        //! \param[in] implementation The new implementation
        void SetImplementation(AZStd::unique_ptr<Implementation> impl) { m_pimpl = AZStd::move(impl); }

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Private pointer to the platform specific implementation
        AZStd::unique_ptr<Implementation> m_pimpl;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Helper class that handles requests to create a custom implementation for this device
        InputDeviceImplementationRequestHandler<InputDeviceKeyboard> m_implementationRequestHandler;
    };
} // namespace AzFramework
