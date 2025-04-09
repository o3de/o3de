/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformIncl.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <xinput.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace XInput
{
    AZStd::shared_ptr<AZ::DynamicModuleHandle> LoadDynamicModule();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for Windows game-pad input devices
    class InputDeviceGamepadWindows : public InputDeviceGamepad::Implementation
                                    , public RawInputNotificationBusWindows::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceGamepadWindows, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        //! \param[in] xinputModuleHandle Shared pointer to the xinput dynamic module
        InputDeviceGamepadWindows(InputDeviceGamepad& inputDevice,
                                  AZStd::shared_ptr<AZ::DynamicModuleHandle> xinputModuleHandle);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceGamepadWindows() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::SetVibration
        void SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::GetPhysicalKeyOrButtonText
        bool GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                        AZStd::string& o_keyOrButtonText) const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsWindows::OnRawInputDeviceChangeEvent
        void OnRawInputDeviceChangeEvent() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::shared_ptr<AZ::DynamicModuleHandle> m_xinputModuleHandle; //!< Handle to the xinput module
        RawGamepadState                            m_rawGamepadState;    //!< The last known raw game-pad state
        bool                                       m_isConnected{};      //!< Is this game-pad currently connected?
        bool                                       m_tryConnect{};       //!< Check whether this game-pad just connected?
    };

} // namespace AzFramework
