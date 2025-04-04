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
    // (implemented in Code/Framework/AzFramework/Platform/iOS/AzFramework/Application/Application_iOS.mm)

    // void NativeUISystemComponent::InitializeDeviceGamepadImplentationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/iOS/AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_iOS.mm)

    void NativeUISystemComponent::InitializeDeviceKeyboardImplementationFactory()
    {
        // Keyboard Input not supported on iOS
    }

    // void NativeUISystemComponent::InitializeDeviceMotionImplentationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/iOS/AzFramework/Input/Devices/Motion/InputDeviceMotion_iOS.mm)

    void NativeUISystemComponent::InitializeDeviceMouseImplentationFactory()
    {
        // Mouse Input not supported on iOS
    }

    // void NativeUISystemComponent::InitializeDeviceTouchImplentationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/iOS/AzFramework/Input/Devices/Touch/InputDeviceTouch_iOS.mm)

    // void NativeUISystemComponent::InitializeDeviceVirtualKeyboardImplentationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/iOS/AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard_iOS.mm)

    // void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    // (implemented in Code/Framework/AzFramework/Platform/iOS/AzFramework/Windowing/NativeWindow_ios.mm)

} // namespace AzFramework
