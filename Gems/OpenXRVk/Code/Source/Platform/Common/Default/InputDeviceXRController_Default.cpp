/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenXRVk/InputDeviceXRController.h>
#include "OculusTouch_Default.h"

namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceXRController::Implementation* InputDeviceXRController::Implementation::Create(
        InputDeviceXRController& inputDevice)
    {
        // Future versions of this function may be able to select from a variety of different device
        // types and do so based on knowledge of the hardware, but for now force the Oculus Touch controller.
        return aznew OpenXRVk::InputDeviceOculusTouch(inputDevice);
    }

} // namespace AzFramework
