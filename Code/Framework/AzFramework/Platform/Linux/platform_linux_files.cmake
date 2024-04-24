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
    AzFramework/Asset/AssetSystemComponentHelper_Linux.cpp
    AzFramework/Process/ProcessUtils_Linux.cpp
    AzFramework/Process/ProcessWatcher_Linux.cpp
    AzFramework/Process/ProcessCommon.h
    AzFramework/Process/ProcessCommunicator_Linux.cpp
    ../Common/UnixLike/AzFramework/Device/DeviceAttributesCommon_UnixLike.cpp
    ../Common/UnixLike/AzFramework/IO/LocalFileIO_UnixLike.cpp
    ../Common/Unimplemented/AzFramework/StreamingInstall/StreamingInstall_Unimplemented.cpp
    ../Common/Default/AzFramework/TargetManagement/TargetManagementComponent_Default.cpp
    AzFramework/Input/User/LocalUserId_Platform.h
    ../Common/Default/AzFramework/Input/User/LocalUserId_Default.h
    AzFramework/Input/LibEVDevWrapper.h
    AzFramework/Input/LibEVDevWrapper.cpp
    AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Linux.cpp
    AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Linux.h
)
