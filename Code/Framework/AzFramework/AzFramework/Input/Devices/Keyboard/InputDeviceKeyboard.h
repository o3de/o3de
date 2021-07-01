/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            static const InputChannelId Alphanumeric0; //!< The 0 key
            static const InputChannelId Alphanumeric1; //!< The 1 key
            static const InputChannelId Alphanumeric2; //!< The 2 key
            static const InputChannelId Alphanumeric3; //!< The 3 key
            static const InputChannelId Alphanumeric4; //!< The 4 key
            static const InputChannelId Alphanumeric5; //!< The 5 key
            static const InputChannelId Alphanumeric6; //!< The 6 key
            static const InputChannelId Alphanumeric7; //!< The 7 key
            static const InputChannelId Alphanumeric8; //!< The 8 key
            static const InputChannelId Alphanumeric9; //!< The 9 key
            static const InputChannelId AlphanumericA; //!< The A key
            static const InputChannelId AlphanumericB; //!< The B key
            static const InputChannelId AlphanumericC; //!< The C key
            static const InputChannelId AlphanumericD; //!< The D key
            static const InputChannelId AlphanumericE; //!< The E key
            static const InputChannelId AlphanumericF; //!< The F key
            static const InputChannelId AlphanumericG; //!< The G key
            static const InputChannelId AlphanumericH; //!< The H key
            static const InputChannelId AlphanumericI; //!< The I key
            static const InputChannelId AlphanumericJ; //!< The J key
            static const InputChannelId AlphanumericK; //!< The K key
            static const InputChannelId AlphanumericL; //!< The L key
            static const InputChannelId AlphanumericM; //!< The M key
            static const InputChannelId AlphanumericN; //!< The N key
            static const InputChannelId AlphanumericO; //!< The O key
            static const InputChannelId AlphanumericP; //!< The P key
            static const InputChannelId AlphanumericQ; //!< The Q key
            static const InputChannelId AlphanumericR; //!< The R key
            static const InputChannelId AlphanumericS; //!< The S key
            static const InputChannelId AlphanumericT; //!< The T key
            static const InputChannelId AlphanumericU; //!< The U key
            static const InputChannelId AlphanumericV; //!< The V key
            static const InputChannelId AlphanumericW; //!< The W key
            static const InputChannelId AlphanumericX; //!< The X key
            static const InputChannelId AlphanumericY; //!< The Y key
            static const InputChannelId AlphanumericZ; //!< The Z key

            // Edit (and escape) Keys
            static const InputChannelId EditBackspace; //!< The backspace key
            static const InputChannelId EditCapsLock;  //!< The caps lock key
            static const InputChannelId EditEnter;     //!< The enter/return key
            static const InputChannelId EditSpace;     //!< The spacebar key
            static const InputChannelId EditTab;       //!< The tab key
            static const InputChannelId Escape;        //!< The escape key

            // Function Keys
            static const InputChannelId Function01; //!< The F1 key
            static const InputChannelId Function02; //!< The F2 key
            static const InputChannelId Function03; //!< The F3 key
            static const InputChannelId Function04; //!< The F4 key
            static const InputChannelId Function05; //!< The F5 key
            static const InputChannelId Function06; //!< The F6 key
            static const InputChannelId Function07; //!< The F7 key
            static const InputChannelId Function08; //!< The F8 key
            static const InputChannelId Function09; //!< The F9 key
            static const InputChannelId Function10; //!< The F10 key
            static const InputChannelId Function11; //!< The F11 key
            static const InputChannelId Function12; //!< The F12 key
            static const InputChannelId Function13; //!< The F13 key
            static const InputChannelId Function14; //!< The F14 key
            static const InputChannelId Function15; //!< The F15 key
            static const InputChannelId Function16; //!< The F16 key
            static const InputChannelId Function17; //!< The F17 key
            static const InputChannelId Function18; //!< The F18 key
            static const InputChannelId Function19; //!< The F19 key
            static const InputChannelId Function20; //!< The F20 key

            // Modifier Keys
            static const InputChannelId ModifierAltL;   //!< The left alt/option key
            static const InputChannelId ModifierAltR;   //!< The right alt/option key
            static const InputChannelId ModifierCtrlL;  //!< The left control key
            static const InputChannelId ModifierCtrlR;  //!< The right control key
            static const InputChannelId ModifierShiftL; //!< The left shift key
            static const InputChannelId ModifierShiftR; //!< The right shift key
            static const InputChannelId ModifierSuperL; //!< The left super (windows or apple) key
            static const InputChannelId ModifierSuperR; //!< The right super (windows or apple) key

            // Navigation Keys
            static const InputChannelId NavigationArrowDown;  //!< The down arrow key
            static const InputChannelId NavigationArrowLeft;  //!< The left arrow key
            static const InputChannelId NavigationArrowRight; //!< The right arrow key
            static const InputChannelId NavigationArrowUp;    //!< The up arrow key
            static const InputChannelId NavigationDelete;     //!< The delete key
            static const InputChannelId NavigationEnd;        //!< The end key
            static const InputChannelId NavigationHome;       //!< The home key
            static const InputChannelId NavigationInsert;     //!< The insert key
            static const InputChannelId NavigationPageDown;   //!< The page down key
            static const InputChannelId NavigationPageUp;     //!< The page up key

            // Numpad Keys
            static const InputChannelId NumLock; //!< The num lock key (the clear key on apple keyboards)
            static const InputChannelId NumPad0; //!< The numpad 0 key
            static const InputChannelId NumPad1; //!< The numpad 1 key
            static const InputChannelId NumPad2; //!< The numpad 2 key
            static const InputChannelId NumPad3; //!< The numpad 3 key
            static const InputChannelId NumPad4; //!< The numpad 4 key
            static const InputChannelId NumPad5; //!< The numpad 5 key
            static const InputChannelId NumPad6; //!< The numpad 6 key
            static const InputChannelId NumPad7; //!< The numpad 7 key
            static const InputChannelId NumPad8; //!< The numpad 8 key
            static const InputChannelId NumPad9; //!< The numpad 9 key
            static const InputChannelId NumPadAdd;      //!< The numpad add key
            static const InputChannelId NumPadDecimal;  //!< The numpad decimal key
            static const InputChannelId NumPadDivide;   //!< The numpad divide key
            static const InputChannelId NumPadEnter;    //!< The numpad enter key
            static const InputChannelId NumPadMultiply; //!< The numpad multiply key
            static const InputChannelId NumPadSubtract; //!< The numpad subtract key

            // Punctuation Keys
            static const InputChannelId PunctuationApostrophe; //!< The apostrophe key
            static const InputChannelId PunctuationBackslash;  //!< The backslash key
            static const InputChannelId PunctuationBracketL;   //!< The left bracket key
            static const InputChannelId PunctuationBracketR;   //!< The right bracket key
            static const InputChannelId PunctuationComma;      //!< The comma key
            static const InputChannelId PunctuationEquals;     //!< The equals key
            static const InputChannelId PunctuationHyphen;     //!< The hyphen/underscore key
            static const InputChannelId PunctuationPeriod;     //!< The period key
            static const InputChannelId PunctuationSemicolon;  //!< The semicolon key
            static const InputChannelId PunctuationSlash;      //!< The (forward) slash key
            static const InputChannelId PunctuationTilde;      //!< The tilde/grave key

            // Supplementary ISO Key
            static const InputChannelId SupplementaryISO;      //!< The supplementary ISO layout key

            // Windows System Keys
            static const InputChannelId WindowsSystemPause;      //!< The windows pause key
            static const InputChannelId WindowsSystemPrint;      //!< The windows print key
            static const InputChannelId WindowsSystemScrollLock; //!< The windows scroll lock key

            //!< All keyboard key ids
            static const AZStd::array<InputChannelId, 112> All;
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
        InputDeviceKeyboard();

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
