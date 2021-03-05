#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    AzFramework/AzFramework_Traits_Platform.h
    AzFramework/AzFramework_Traits_Windows.h
    AzFramework/API/ApplicationAPI_Platform.h
    AzFramework/API/ApplicationAPI_Windows.h
    AzFramework/Application/Application_Windows.cpp
    AzFramework/Asset/AssetSystemComponentHelper_Windows.cpp
    ../Common/WinAPI/AzFramework/IO/LocalFileIO_WinAPI.cpp
    AzFramework/IO/LocalFileIO_Windows.cpp
    ../Common/WinAPI/AzFramework/Network/AssetProcessorConnection_WinAPI.cpp
    ../Common/Unimplemented/AzFramework/StreamingInstall/StreamingInstall_Unimplemented.cpp
    AzFramework/TargetManagement/TargetManagementComponent_Windows.cpp
    AzFramework/Windowing/NativeWindow_Windows.cpp
    AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h
    AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Windows.h
    AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Windows.cpp
    AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_Windows.cpp
    ../Common/WinAPI/AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_WinAPI.h
    ../Common/Unimplemented/AzFramework/Input/Devices/Motion/InputDeviceMotion_Unimplemented.cpp
    AzFramework/Input/Devices/Mouse/InputDeviceMouse_Windows.cpp
    ../Common/Unimplemented/AzFramework/Input/Devices/Touch/InputDeviceTouch_Unimplemented.cpp
    AzFramework/Input/User/LocalUserId_Platform.h
    ../Common/Default/AzFramework/Input/User/LocalUserId_Default.h
    ../Common/Unimplemented/AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard_Unimplemented.cpp
    AzFramework/Archive/ArchiveVars_Platform.h
    AzFramework/Archive/ArchiveVars_Windows.h
)
