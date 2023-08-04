/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Windows.h>
#include <AzFramework/Input/Buses/Requests/InputTextEntryRequestBus.h>
#include <AzFramework/Input/Channels/InputChannelId.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <../Common/WinAPI/AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_WinAPI.h>


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
        static AZ::EnvironmentVariable<int> s_instanceCount;

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceKeyboardWindows, AZ::SystemAllocator);

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

} // namespace AzFramework
