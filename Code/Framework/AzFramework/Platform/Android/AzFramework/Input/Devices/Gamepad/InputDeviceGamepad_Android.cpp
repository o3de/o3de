/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for android game-pad input devices
    class InputDeviceGamepadAndroid : public InputDeviceGamepad::Implementation
                                    , public RawInputNotificationBusAndroid::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceGamepadAndroid, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceGamepadAndroid(InputDeviceGamepad& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceGamepadAndroid() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::SetVibration
        void SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsAndroid::OnRawInputEvent
        void OnRawInputEvent(const AInputEvent* rawInputEvent) override;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::u32 InputDeviceGamepad::GetMaxSupportedGamepads()
    {
        // ToDo: Return the maximum number of supported gamepads
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepad::Implementation* InputDeviceGamepad::Implementation::Create(
        InputDeviceGamepad& inputDevice)
    {
        return aznew InputDeviceGamepadAndroid(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepadAndroid::InputDeviceGamepadAndroid(InputDeviceGamepad& inputDevice)
        : InputDeviceGamepad::Implementation(inputDevice)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepadAndroid::~InputDeviceGamepadAndroid()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGamepadAndroid::IsConnected() const
    {
        // ToDo: Figure out how/whether we can determine the availability of this game-pad,
        // and implement this function (along with dispatching connect/disconnect events)
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepadAndroid::SetVibration(float leftMotorSpeedNormalized,
                                                 float rightMotorSpeedNormalized)
    {
        // ToDo: Implement force-feedback if supported?
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepadAndroid::TickInputDevice()
    {
        // ToDo: Process raw game-pad input and update input channels
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepadAndroid::OnRawInputEvent(const AInputEvent* rawInputEvent)
    {
        // ToDo: Process raw game-pad input events
    }
} // namespace AzFramework
