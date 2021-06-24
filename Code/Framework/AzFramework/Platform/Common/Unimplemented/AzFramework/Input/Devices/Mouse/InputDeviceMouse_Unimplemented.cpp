/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouse::Implementation* InputDeviceMouse::Implementation::Create(InputDeviceMouse&)
    {
        return nullptr;
    }
} // namespace AzFramework
