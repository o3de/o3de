/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Motion/InputDeviceMotion.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMotion::Implementation* InputDeviceMotion::Implementation::Create(InputDeviceMotion&)
    {
        return nullptr;
    }
} // namespace AzFramework
