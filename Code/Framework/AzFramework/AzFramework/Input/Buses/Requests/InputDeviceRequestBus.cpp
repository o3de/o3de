/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Buses/Requests/InputDeviceRequestBus.h>

DECLARE_EBUS_INSTANTIATION(AzFramework::InputDeviceRequests);

namespace AzFramework
{
    // This function is declared in the implementation file to avoid inclusion of
    // InputDeviceRequestBus instantiating the bus itself.
    const InputDevice* InputDeviceRequests::FindInputDevice(const InputDeviceId& deviceId)
    {
        const InputDevice* inputDevice = nullptr;
        InputDeviceRequestBus::EventResult(inputDevice, deviceId, &InputDeviceRequests::GetInputDevice);
        return inputDevice;
    }
}
