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
    AzFramework/AzFramework_Traits_Android.h
    AzFramework/API/ApplicationAPI_Platform.h
    AzFramework/API/ApplicationAPI_Android.h
    AzFramework/Application/Application_Android.cpp
    ../Common/Unimplemented/AzFramework/Asset/AssetSystemComponentHelper_Unimplemented.cpp
    AzFramework/IO/LocalFileIO_Android.cpp
    ../Common/Default/AzFramework/Network/AssetProcessorConnection_Default.cpp
    ../Common/Unimplemented/AzFramework/StreamingInstall/StreamingInstall_Unimplemented.cpp
    ../Common/Default/AzFramework/TargetManagement/TargetManagementComponent_Default.cpp
    AzFramework/Windowing/NativeWindow_Android.cpp
    AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h
    AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Android.h
    AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Android.cpp
    AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_Android.cpp
    AzFramework/Input/Devices/Motion/InputDeviceMotion_Android.cpp
    AzFramework/Input/Devices/Mouse/InputDeviceMouse_Android.cpp
    AzFramework/Input/Devices/Touch/InputDeviceTouch_Android.cpp
    AzFramework/Input/User/LocalUserId_Platform.h
    ../Common/Default/AzFramework/Input/User/LocalUserId_Default.h
    AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard_Android.cpp
    AzFramework/Archive/ArchiveVars_Platform.h
    AzFramework/Archive/ArchiveVars_Android.h
    AzFramework/Process/ProcessCommon.h
    AzFramework/Process/ProcessWatcher_Android.cpp
    AzFramework/Process/ProcessCommunicator_Android.cpp
)
