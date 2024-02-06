/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformIncl.h>
#include <AzCore/Time/ITime.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>

namespace GamepadLinuxPrivate
{
     class InternalState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    struct LibEVDevWrapper;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for Linux game-pad input devices
    class InputDeviceGamepadLinux : public InputDeviceGamepad::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceGamepadLinux, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceGamepadLinux(InputDeviceGamepad& inputDevice, AZStd::shared_ptr<AzFramework::LibEVDevWrapper> libevdevWrapper);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceGamepadLinux() override;

        static InputDeviceGamepad::Implementation* Create(InputDeviceGamepad& inputDevice, AZStd::shared_ptr<AzFramework::LibEVDevWrapper> libevdevWrapper);

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

        void TryConnect();
        void BumpTryAgainTimeout(); //! Bumps the timeout of m_tryAgainTimeout to wait a little while.
        void UpdateButtonState(AZ::u32 buttonMask, bool pressed);

        // Variables
        RawGamepadState                            m_rawGamepadState;    //!< The last known raw game-pad state
        bool                                       m_isConnected{};      //!< Is this game-pad currently connected?
        AZ::TimeMs                                 m_tryAgainTimeout = AZ::Time::ZeroTimeMs;//!< Timeout before trying to connect again

        AZStd::unique_ptr<GamepadLinuxPrivate::InternalState> m_internalState;
        
    };

} // namespace AzFramework
