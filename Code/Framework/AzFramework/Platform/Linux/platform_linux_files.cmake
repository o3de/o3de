#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    AzFramework/AzFramework_Traits_Platform.h
    AzFramework/AzFramework_Traits_Linux.h
    AzFramework/API/ApplicationAPI_Platform.h
    AzFramework/API/ApplicationAPI_Linux.h
    AzFramework/Application/Application_Linux.cpp
    AzFramework/Asset/AssetSystemComponentHelper_Linux.cpp
    AzFramework/Process/ProcessWatcher_Linux.cpp
    AzFramework/Process/ProcessCommon.h
    AzFramework/Process/ProcessCommunicator_Linux.cpp
    ../Common/UnixLike/AzFramework/IO/LocalFileIO_UnixLike.cpp
    ../Common/Unimplemented/AzFramework/StreamingInstall/StreamingInstall_Unimplemented.cpp
    ../Common/Default/AzFramework/TargetManagement/TargetManagementComponent_Default.cpp
    AzFramework/Windowing/NativeWindow_Linux.cpp
    ../Common/Unimplemented/AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Unimplemented.cpp
    AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_Linux.cpp
    AzFramework/Input/Devices/Mouse/InputDeviceMouse_Linux.cpp
    ../Common/Unimplemented/AzFramework/Input/Devices/Motion/InputDeviceMotion_Unimplemented.cpp
    ../Common/Unimplemented/AzFramework/Input/Devices/Touch/InputDeviceTouch_Unimplemented.cpp
    AzFramework/Input/User/LocalUserId_Platform.h
    ../Common/Default/AzFramework/Input/User/LocalUserId_Default.h
    ../Common/Unimplemented/AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard_Unimplemented.cpp
    AzFramework/Archive/ArchiveVars_Platform.h
    AzFramework/Archive/ArchiveVars_Linux.h
)
