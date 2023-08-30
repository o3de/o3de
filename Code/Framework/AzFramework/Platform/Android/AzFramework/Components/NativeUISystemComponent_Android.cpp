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

    void NativeUISystemComponent::InitializeDeviceGamepadImplentationFactory()
    {
        // Gamepad input not supported on Android
    }

    // void NativeUISystemComponent::InitializeApplicationImplementationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Android/AzFramework/Application/Application_Android.cpp)

    // void NativeUISystemComponent::InitializeDeviceKeyboardImplementationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Android/AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_Android.cpp)

    // void NativeUISystemComponent::InitializeDeviceMotionImplentationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Android/AzFramework/Input/Devices/Motion/InputDeviceMotion_Android.cpp)

    // void NativeUISystemComponent::InitializeDeviceMouseImplentationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Android/AzFramework/Input/Devices/Mouse/InputDeviceMouse_Android.cpp)

    // void NativeUISystemComponent::InitializeDeviceTouchImplentationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Android/AzFramework/Input/Devices/Touch/InputDeviceTouch_Android.cpp)

    // void NativeUISystemComponent::InitializeDeviceVirtualKeyboardImplentationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Android/AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard_Android.cpp)

    // void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/Android/AzFramework/Windowing/NativeWindow_Android.cpp)

} // namespace AzFramework
