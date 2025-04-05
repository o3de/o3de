#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    ../Common/WinAPI/AzCore/IO/FileIO_WinAPI.cpp
    AzCore/IO/SystemFile_Platform.h
    AzCore/IO/Streamer/StorageDrive_Windows.h
    AzCore/IO/Streamer/StorageDrive_Windows.cpp
    AzCore/IO/Streamer/StorageDriveConfig_Windows.h
    AzCore/IO/Streamer/StorageDriveConfig_Windows.cpp
    AzCore/IO/Streamer/StreamerConfiguration_Windows.h
    AzCore/IO/Streamer/StreamerConfiguration_Windows.cpp
    AzCore/IO/Streamer/StreamerContext_Platform.h
    AzCore/IPC/SharedMemory_Platform.h
    AzCore/IPC/SharedMemory_Windows.h
    AzCore/IPC/SharedMemory_Windows.cpp
    AzCore/Math/Random_Platform.h
    AzCore/Module/Internal/ModuleManagerSearchPathTool_Windows.cpp
    AzCore/NativeUI/NativeUISystemComponent_Windows.cpp
    AzCore/Socket/AzSocket_Platform.h
    AzCore/Socket/AzSocket_fwd_Platform.h
    AzCore/Socket/AzSocket_fwd_Windows.h
    AzCore/Utils/Utils_Windows.cpp
    ../Common/WinAPI/AzCore/IO/SystemFile_WinAPI.cpp
    ../Common/WinAPI/AzCore/IO/SystemFile_WinAPI.h
    ../Common/WinAPI/AzCore/Module/DynamicModuleHandle_WinAPI.cpp
    ../Common/WinAPI/AzCore/Process/ProcessInfo_WinAPI.cpp
    ../Common/WinAPI/AzCore/Socket/AzSocket_fwd_WinAPI.h
    ../Common/WinAPI/AzCore/Socket/AzSocket_WinAPI.cpp
    ../Common/WinAPI/AzCore/Socket/AzSocket_WinAPI.h
    ../Common/WinAPI/AzCore/Utils/Utils_WinAPI.cpp
)
