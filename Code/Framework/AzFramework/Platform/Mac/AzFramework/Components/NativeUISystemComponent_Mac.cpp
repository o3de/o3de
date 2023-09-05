/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/NativeUISystemComponent.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    // void NativeUISystemComponent::InitializeApplicationImplementationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Mac/AzFramework/Application/Application_Mac.mm)

    // void NativeUISystemComponent::InitializeDeviceGamepadImplentationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Mac/AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Mac.mm)

    // void NativeUISystemComponent::InitializeDeviceKeyboardImplementationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Mac/AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_Mac.mm)

    void NativeUISystemComponent::InitializeDeviceMotionImplentationFactory()
    {
        // Motion Input not supported on Mac
    }

    // void NativeUISystemComponent::InitializeDeviceMouseImplentationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Mac/AzFramework/Input/Devices/Mouse/InputDeviceMouse_Mac.mm)

    void NativeUISystemComponent::InitializeDeviceTouchImplentationFactory()
    {
        // Touch Input not supported on Mac
    }

    void NativeUISystemComponent::InitializeDeviceVirtualKeyboardImplentationFactory()
    {
        // Virtual Keyboard not supported on Mac
    }

    // void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Mac/AzFramework/Windowing/NativeWindow_Mac.mm)
} // namespace AzFramework
