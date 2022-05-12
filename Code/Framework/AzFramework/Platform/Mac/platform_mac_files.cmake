#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    AzFramework/AzFramework_Traits_Platform.h
    AzFramework/AzFramework_Traits_Mac.h
    AzFramework/API/ApplicationAPI_Platform.h
    AzFramework/API/ApplicationAPI_Mac.h
    AzFramework/Application/Application_Mac.mm
    AzFramework/Asset/AssetSystemComponentHelper_Mac.cpp
    AzFramework/Process/ProcessWatcher_Mac.cpp
    AzFramework/Process/ProcessCommon.h
    AzFramework/Process/ProcessCommunicator_Mac.cpp
    ../Common/UnixLike/AzFramework/IO/LocalFileIO_UnixLike.cpp
    ../Common/Unimplemented/AzFramework/StreamingInstall/StreamingInstall_Unimplemented.cpp
    AzFramework/TargetManagement/TargetManagementComponent_Mac.cpp
    AzFramework/Windowing/NativeWindow_Mac.mm
    AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h
    AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Mac.h
    ../Common/Apple/AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Apple.mm
    AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_Mac.mm
    ../Common/Unimplemented/AzFramework/Input/Devices/Motion/InputDeviceMotion_Unimplemented.cpp
    AzFramework/Input/Devices/Mouse/InputDeviceMouse_Mac.mm
    ../Common/Unimplemented/AzFramework/Input/Devices/Touch/InputDeviceTouch_Unimplemented.cpp
    AzFramework/Input/User/LocalUserId_Platform.h
    ../Common/Default/AzFramework/Input/User/LocalUserId_Default.h
    ../Common/Unimplemented/AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard_Unimplemented.cpp
    ../Common/Apple/AzFramework/Utils/SystemUtilsApple.h
    ../Common/Apple/AzFramework/Utils/SystemUtilsApple.mm
)
