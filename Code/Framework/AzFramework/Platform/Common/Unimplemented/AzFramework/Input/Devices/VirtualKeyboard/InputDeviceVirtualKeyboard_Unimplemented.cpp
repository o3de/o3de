/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboard::Implementation* InputDeviceVirtualKeyboard::Implementation::Create(
        InputDeviceVirtualKeyboard&)
    {
        return nullptr;
    }
} // namespace AzFramework
