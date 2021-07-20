/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <../Common/WinAPI/AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_WinAPI.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    // These are scarcely documented, and do not seem to be publically accessible:
    // - https://msdn.microsoft.com/en-us/library/windows/desktop/ms645546(v=vs.85).aspx
    // - https://msdn.microsoft.com/en-us/library/ff543440.aspx
    const USHORT RAW_INPUT_KEYBOARD_USAGE_PAGE = 0x01;
    const USHORT RAW_INPUT_KEYBOARD_USAGE = 0x06;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for Windows keyboard input devices
    class InputDeviceKeyboardWindows : public InputDeviceKeyboard::Implementation
                                     , public RawInputNotificationBusWindows::Handler
    {
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Count of the number instances of this class that have been created
        static int s_instanceCount;

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceKeyboardWindows, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceKeyboardWindows(InputDeviceKeyboard& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceKeyboardWindows() override;

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
        //! \ref AzFramework::InputDeviceKeyboard::Implementation::GetPhysicalKeyOrButtonText
        void GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                        AZStd::string& o_keyOrButtonText) const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsWindows::OnRawInputEvent
        void OnRawInputEvent(const RAWINPUT& rawInput) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsWindows::OnRawInputCodeUnitUTF16Event
        void OnRawInputCodeUnitUTF16Event(uint16_t codeUnitUTF16) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! If a codeUnitUTF16 param passed to OnRawInputCodeUnitUTF16Event is part of a 2 code-unit
        //! sequence (ie. it doesn't correspond directly to a single UTF16 code-point) we must store
        //! the 'lead surrogate' so the subsequent 'trailing surrogate' can be correctly interpreted.
        UTF16ToUTF8Converter m_UTF16ToUTF8Converter;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Cached map of Windows scan codes indexed by their corresponding input channel id
        AZStd::unordered_map<InputChannelId, AZ::u32> m_scanCodesByInputChannelId;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Does the window attached to the input (main) thread's message queue have focus?
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms646294(v=vs.85).aspx
        bool m_hasFocus = false;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Has text entry been started?
        bool m_hasTextEntryStarted = false;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboard::Implementation* InputDeviceKeyboard::Implementation::Create(InputDeviceKeyboard& inputDevice)
    {
        return aznew InputDeviceKeyboardWindows(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    int InputDeviceKeyboardWindows::s_instanceCount = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboardWindows::InputDeviceKeyboardWindows(InputDeviceKeyboard& inputDevice)
        : InputDeviceKeyboard::Implementation(inputDevice)
        , m_UTF16ToUTF8Converter()
        , m_scanCodesByInputChannelId(ConstructScanCodeByInputChannelIdMap())
        , m_hasFocus(false)
        , m_hasTextEntryStarted(false)
    {
        if (s_instanceCount++ == 0)
        {
            // Register for raw keyboard input
            RAWINPUTDEVICE rawInputDevice;
            rawInputDevice.usUsagePage = RAW_INPUT_KEYBOARD_USAGE_PAGE;
            rawInputDevice.usUsage     = RAW_INPUT_KEYBOARD_USAGE;
            rawInputDevice.dwFlags     = 0;
            rawInputDevice.hwndTarget  = 0;
            const BOOL result = RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice));
            AZ_Assert(result, "Failed to register raw input device: keyboard");
            AZ_UNUSED(result);
        }

        RawInputNotificationBusWindows::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboardWindows::~InputDeviceKeyboardWindows()
    {
        RawInputNotificationBusWindows::Handler::BusDisconnect();

        if (--s_instanceCount == 0)
        {
            // Deregister from raw keyboard input
            RAWINPUTDEVICE rawInputDevice;
            rawInputDevice.usUsagePage = RAW_INPUT_KEYBOARD_USAGE_PAGE;
            rawInputDevice.usUsage     = RAW_INPUT_KEYBOARD_USAGE;
            rawInputDevice.dwFlags     = RIDEV_REMOVE;
            rawInputDevice.hwndTarget  = 0;
            const BOOL result = RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice));
            AZ_Assert(result, "Failed to deregister raw input device: keyboard");
            AZ_UNUSED(result);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboardWindows::IsConnected() const
    {
        // If necessary, we can register raw input devices using RIDEV_DEVNOTIFY in order to receive
        // WM_INPUT_DEVICE_CHANGE Windows messages in the WndProc function. These could then be sent
        // using an EBus (RawInputNotificationBusWindows?) and used to keep track of connected state.
        //
        // Doing this would allow (in one respect force) us to distinguish between multiple physical
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
        // InputDeviceKeyboardWindows::OnRawInputEvent function to filter incoming events by raw id.
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboardWindows::HasTextEntryStarted() const
    {
        return m_hasTextEntryStarted;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardWindows::TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions&)
    {
        m_hasTextEntryStarted = true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardWindows::TextEntryStop()
    {
        m_hasTextEntryStarted = false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardWindows::TickInputDevice()
    {
        // The input event loop is pumped by the system on Windows so all raw input events for this
        // frame have already been dispatched. But they are all queued until ProcessRawEventQueues
        // is called below so that all raw input events are processed at the same time every frame.

        const bool hadFocus = m_hasFocus;
        m_hasFocus = ::GetFocus() != nullptr;
        if (m_hasFocus)
        {
            // Process raw event queues once each frame while this thread's message queue has focus
            ProcessRawEventQueues();
            if (!hadFocus)
            {
                // If we just gained focus, reset state after processing any events that are queued so that we don't have stale state lying around
                ResetInputChannelStates();
            }
        }
        else if (hadFocus)
        {
            // The window attached to this thread's message queue no longer has focus, process any
            // events that are queued, before resetting the state of all associated input channels.
            ProcessRawEventQueues();
            ResetInputChannelStates();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardWindows::GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                                                AZStd::string& o_keyOrButtonText) const
    {
        const auto& it = m_scanCodesByInputChannelId.find(inputChannelId);
        if (it == m_scanCodesByInputChannelId.end())
        {
            return;
        }

        static const int bufferLength = 64;
        WCHAR buffer[bufferLength];
        const AZ::u32 scanCode = it->second;
        LONG lParam = scanCode << 16;
        const int stringLength = GetKeyNameTextW(lParam, buffer, bufferLength);
        if (stringLength != 0)
        {
            // Convert UTF-16 to UTF-8
            AZStd::to_string(o_keyOrButtonText, buffer, stringLength);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardWindows::OnRawInputEvent(const RAWINPUT& rawInput)
    {
        if (rawInput.header.dwType != RIM_TYPEKEYBOARD || !::GetFocus())
        {
            return;
        }

        const RAWKEYBOARD& rawKeyboardData = rawInput.data.keyboard;
        const AZ::u32 scanCode = rawKeyboardData.MakeCode;
        const AZ::u32 virtualKeyCode = rawKeyboardData.VKey;
        const bool hasExtendedKeyPrefix = ((rawKeyboardData.Flags & RI_KEY_E0) != 0);
        const InputChannelId* channelId = GetInputChannelIdFromRawKeyEvent(scanCode,
                                                                           virtualKeyCode,
                                                                           hasExtendedKeyPrefix);
        if (channelId)
        {
            const bool isKeyPressed = ((rawKeyboardData.Flags & RI_KEY_BREAK) == 0);
            QueueRawKeyEvent(*channelId, isKeyPressed);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardWindows::OnRawInputCodeUnitUTF16Event(uint16_t codeUnitUTF16)
    {
        const AZStd::string codePointUTF8 = m_UTF16ToUTF8Converter.FeedCodeUnitUTF16(codeUnitUTF16);

#if !defined(ALWAYS_DISPATCH_KEYBOARD_TEXT_INPUT)
        if (!m_hasTextEntryStarted)
        {
            return;
        }
#endif // !defined(ALWAYS_DISPATCH_KEYBOARD_TEXT_INPUT)

        if (!codePointUTF8.empty())
        {
            QueueRawTextEvent(codePointUTF8);
        }
    }
} // namespace AzFramework
