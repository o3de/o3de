#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    AzFramework/AzFramework_Traits_Platform.h
    AzFramework/AzFramework_Traits_Windows.h
    AzFramework/API/ApplicationAPI_Platform.h
    AzFramework/API/ApplicationAPI_Windows.h
    AzFramework/Application/Application_Windows.cpp
    AzFramework/Asset/AssetSystemComponentHelper_Windows.cpp
    AzFramework/Process/ProcessWatcher_Win.cpp
    AzFramework/Process/ProcessCommon.h
    AzFramework/Process/ProcessCommunicator_Win.cpp
    AzFramework/Process/ProcessUtils_Win.cpp
    ../Common/WinAPI/AzFramework/IO/LocalFileIO_WinAPI.cpp
    AzFramework/IO/LocalFileIO_Windows.cpp
    ../Common/Unimplemented/AzFramework/StreamingInstall/StreamingInstall_Unimplemented.cpp
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
)
