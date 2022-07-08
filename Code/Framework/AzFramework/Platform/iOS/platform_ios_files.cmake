#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    AzFramework/AzFramework_Traits_Platform.h
    AzFramework/AzFramework_Traits_iOS.h
    AzFramework/API/ApplicationAPI_Platform.h
    AzFramework/API/ApplicationAPI_iOS.h
    AzFramework/Application/Application_iOS.mm
    ../Common/Unimplemented/AzFramework/Asset/AssetSystemComponentHelper_Unimplemented.cpp
    ../Common/UnixLike/AzFramework/IO/LocalFileIO_UnixLike.cpp
    ../Common/Unimplemented/AzFramework/StreamingInstall/StreamingInstall_Unimplemented.cpp
    ../Common/Default/AzFramework/TargetManagement/TargetManagementComponent_Default.cpp
    AzFramework/Windowing/NativeWindow_ios.mm
    AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h
    AzFramework/Input/Buses/Notifications/RawInputNotificationBus_iOS.h
    ../Common/Apple/AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Apple.mm
    ../Common/Unimplemented/AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_Unimplemented.cpp
    AzFramework/Input/Devices/Motion/InputDeviceMotion_iOS.mm
    ../Common/Unimplemented/AzFramework/Input/Devices/Mouse/InputDeviceMouse_Unimplemented.cpp
    AzFramework/Input/Devices/Touch/InputDeviceTouch_iOS.mm
    AzFramework/Input/User/LocalUserId_Platform.h
    ../Common/Default/AzFramework/Input/User/LocalUserId_Default.h
    ../Common/Apple/AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard_Apple.mm
    AzFramework/Process/ProcessCommon.h
    AzFramework/Process/ProcessWatcher_iOS.cpp
    AzFramework/Process/ProcessCommunicator_iOS.cpp
    ../Common/Apple/AzFramework/Utils/SystemUtilsApple.h
    ../Common/Apple/AzFramework/Utils/SystemUtilsApple.mm
)

