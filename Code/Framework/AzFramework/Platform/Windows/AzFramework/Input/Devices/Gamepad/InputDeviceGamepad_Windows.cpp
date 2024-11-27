/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Windows.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>

#include <xinput.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Map of digital button ids keyed by their xinput button bitmask
    const AZStd::unordered_map<AZ::u32, const InputChannelId*> GetDigitalButtonIdByBitMaskMap()
    {
        const AZStd::unordered_map<AZ::u32, const InputChannelId*> map =
        {
            { XINPUT_GAMEPAD_DPAD_UP,       &InputDeviceGamepad::Button::DU },    // 0x0001
            { XINPUT_GAMEPAD_DPAD_DOWN,     &InputDeviceGamepad::Button::DD },    // 0x0002
            { XINPUT_GAMEPAD_DPAD_LEFT,     &InputDeviceGamepad::Button::DL },    // 0x0004
            { XINPUT_GAMEPAD_DPAD_RIGHT,    &InputDeviceGamepad::Button::DR },    // 0x0008

            { XINPUT_GAMEPAD_START,         &InputDeviceGamepad::Button::Start }, // 0x0010
            { XINPUT_GAMEPAD_BACK,          &InputDeviceGamepad::Button::Select },// 0x0020

            { XINPUT_GAMEPAD_LEFT_THUMB,    &InputDeviceGamepad::Button::L3 },    // 0x0040
            { XINPUT_GAMEPAD_RIGHT_THUMB,   &InputDeviceGamepad::Button::R3 },    // 0x0080

            { XINPUT_GAMEPAD_LEFT_SHOULDER, &InputDeviceGamepad::Button::L1 },    // 0x0100
            { XINPUT_GAMEPAD_RIGHT_SHOULDER,&InputDeviceGamepad::Button::R1 },    // 0x0200

            { XINPUT_GAMEPAD_A,             &InputDeviceGamepad::Button::A },     // 0x1000
            { XINPUT_GAMEPAD_B,             &InputDeviceGamepad::Button::B },     // 0x2000
            { XINPUT_GAMEPAD_X,             &InputDeviceGamepad::Button::X },     // 0x4000
            { XINPUT_GAMEPAD_Y,             &InputDeviceGamepad::Button::Y }      // 0x8000
        };
        return map;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! The maximum value and dead zone of analog trigger buttons
    const float AnalogTriggerMaxValue = 255.0f;
    const float AnalogTriggerDeadZone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! The maximum value and radial dead zones of the left and right thumb-sticks
    const float ThumbStickMaxValue = 32767.0f;
    const float ThumbStickLeftDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    const float ThumbStickRightDeadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! The maximum rotation value of the left (large) and right (small) force-feedback motors
    const float VibrationMaxValue = 65535.0f;
} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Unfortunately, the XInput library doesn't ship with Windows Server 2012, so we're forced to load
//! it explicitly at run-time (instead of simply linking against it), and handle it not being found.
//! https://msdn.microsoft.com/en-us/library/windows/desktop/hh405051(v=vs.85).aspx
namespace XInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Name of the XInput dynamic module
    const char* DynamicModuleName = "XInput9_1_0";

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Weak handle to the XInput dynamic module
    AZStd::weak_ptr<AZ::DynamicModuleHandle> WeakHandleToDynamicModule;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Name of the XInputGetState function
    const char* GetStateFunctionName = "XInputGetState";

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Type/signature of the XInputGetState function
    using GetStateFunctionType = DWORD(*)(DWORD, XINPUT_STATE*);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Function pointer to the XInputGetState function
    GetStateFunctionType GetStateFunctionPointer = nullptr;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Name of the XInputSetState function
    const char* SetStateFunctionName = "XInputSetState";

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Type/signature of the XInputSetState function
    using SetStateFunctionType = DWORD(*)(DWORD, XINPUT_VIBRATION*);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Function pointer to the XInputSetState function
    SetStateFunctionType SetStateFunctionPointer = nullptr;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Load the XInput dynamic module
    //! \return A shared pointer to the XInput dynamic module. May be null if it could not be loaded.
    AZStd::shared_ptr<AZ::DynamicModuleHandle> LoadDynamicModule()
    {
        if (!WeakHandleToDynamicModule.expired())
        {
            return WeakHandleToDynamicModule.lock();
        }

        AZStd::shared_ptr<AZ::DynamicModuleHandle> handle = AZ::DynamicModuleHandle::Create(DynamicModuleName);
        const bool loaded = handle->Load();
        if (!loaded)
        {
            // Could not load XInput9_1_0, this is most likely a Windows Server 2012 machine.
            return nullptr;
        }

        GetStateFunctionPointer = handle->GetFunction<GetStateFunctionType>(GetStateFunctionName);
        if (!GetStateFunctionPointer)
        {
            AZ_Assert(false, "Could not find %s function in %", GetStateFunctionName, DynamicModuleName);
            return nullptr;
        }

        SetStateFunctionPointer = handle->GetFunction<SetStateFunctionType>(SetStateFunctionName);
        if (!SetStateFunctionPointer)
        {
            AZ_Assert(false, "Could not find %s function in %", SetStateFunctionName, DynamicModuleName);
            GetStateFunctionPointer = nullptr;
            return nullptr;
        }

        WeakHandleToDynamicModule = handle;
        return handle;
    }
} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepadWindows::InputDeviceGamepadWindows(InputDeviceGamepad& inputDevice,
                                                         AZStd::shared_ptr<AZ::DynamicModuleHandle> xinputModuleHandle)
        : InputDeviceGamepad::Implementation(inputDevice)
        , m_xinputModuleHandle(xinputModuleHandle)
        , m_rawGamepadState(GetDigitalButtonIdByBitMaskMap())
        , m_isConnected(false)
        , m_tryConnect(true)
    {
        AZ_Assert(m_xinputModuleHandle, "Creating instance of InputDeviceGamepadWindows with a null XInput handle.");

        m_rawGamepadState.m_triggerMaximumValue = AnalogTriggerMaxValue;
        m_rawGamepadState.m_triggerDeadZoneValue = AnalogTriggerDeadZone;
        m_rawGamepadState.m_thumbStickMaximumValue = ThumbStickMaxValue;
        m_rawGamepadState.m_thumbStickLeftDeadZone = ThumbStickLeftDeadZone;
        m_rawGamepadState.m_thumbStickRightDeadZone = ThumbStickRightDeadZone;

        RawInputNotificationBusWindows::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepadWindows::~InputDeviceGamepadWindows()
    {
        RawInputNotificationBusWindows::Handler::BusDisconnect();

        // This basically defeats the purpose of using a weak_ptr in the first place, but we must
        // explicitly call reset on it before the application shuts down so that the shared count
        // object (which was allocated by the system allocator) is deleted before global shutdown.
        m_xinputModuleHandle.reset();
        if (XInput::WeakHandleToDynamicModule.expired())
        {
            XInput::WeakHandleToDynamicModule.reset();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGamepadWindows::IsConnected() const
    {
        return m_isConnected;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepadWindows::SetVibration(float leftMotorSpeedNormalized,
                                                 float rightMotorSpeedNormalized)
    {
        if (m_isConnected)
        {
            XINPUT_VIBRATION vibration;
            vibration.wLeftMotorSpeed = static_cast<WORD>(VibrationMaxValue * AZ::GetClamp(leftMotorSpeedNormalized, 0.0f, 1.0f));
            vibration.wRightMotorSpeed = static_cast<WORD>(VibrationMaxValue * AZ::GetClamp(rightMotorSpeedNormalized, 0.0f, 1.0f));
            XInput::SetStateFunctionPointer(GetInputDeviceIndex(), &vibration);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGamepadWindows::GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                                               AZStd::string& o_keyOrButtonText) const
    {
        if (inputChannelId == InputDeviceGamepad::Button::Select)
        {
            o_keyOrButtonText = "Back";
            return true;
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepadWindows::TickInputDevice()
    {
        // Only process gamepad input while this thread's message queue has focus to
        // keep the behaviour consistent with the mouse and keyboard implementations.
        if (::GetFocus() == nullptr)
        {
            return;
        }

        // Calling XInputGetState every frame for unconnected gamepad devices is extremely slow, but
        // calling XInputGetState every frame for a device that is already connected is much faster.
        // To get around this, unless we're already connected we'll only call XInputGetState once at
        // startup and once each time we're notified of new connections by a WM_DEVICECHANGE message.
        if (!m_isConnected)
        {
            if (!m_tryConnect)
            {
                return;
            }
            m_tryConnect = false;
        }

        XINPUT_STATE newInputState;
        ZeroMemory(&newInputState, sizeof(XINPUT_STATE));

        const AZ::u32 deviceIndex = GetInputDeviceIndex();
        const DWORD result = XInput::GetStateFunctionPointer(deviceIndex, &newInputState);

        if (result == ERROR_SUCCESS)
        {
            if (!m_isConnected)
            {
                // The game-pad connected since the last call to this function
                m_isConnected = true;
                BroadcastInputDeviceConnectedEvent();
            }

            // Always update the input channels while the game-pad is connected
            m_rawGamepadState.m_digitalButtonStates = newInputState.Gamepad.wButtons;
            m_rawGamepadState.m_triggerButtonLState = static_cast<float>(newInputState.Gamepad.bLeftTrigger);
            m_rawGamepadState.m_triggerButtonRState = static_cast<float>(newInputState.Gamepad.bRightTrigger);
            m_rawGamepadState.m_thumbStickLeftXState = static_cast<float>(newInputState.Gamepad.sThumbLX);
            m_rawGamepadState.m_thumbStickLeftYState = static_cast<float>(newInputState.Gamepad.sThumbLY);
            m_rawGamepadState.m_thumbStickRightXState = static_cast<float>(newInputState.Gamepad.sThumbRX);
            m_rawGamepadState.m_thumbStickRightYState = static_cast<float>(newInputState.Gamepad.sThumbRY);
            ProcessRawGamepadState(m_rawGamepadState);
        }
        else if (m_isConnected)
        {
            // The game-pad disconnected since the last call to this function
            m_isConnected = false;
            m_rawGamepadState.Reset();
            ResetInputChannelStates();
            BroadcastInputDeviceDisconnectedEvent();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepadWindows::OnRawInputDeviceChangeEvent()
    {
        // Calling XInputGetState every frame for unconnected gamepad devices is extremely slow, but
        // calling XInputGetState every frame for a device that is already connected is much faster.
        // To get around this, unless we're already connected we'll only call XInputGetState once at
        // startup and once each time we're notified of new connections by a WM_DEVICECHANGE message.
        if (!m_isConnected)
        {
            m_tryConnect = true;
        }
    }
} // namespace AzFramework
