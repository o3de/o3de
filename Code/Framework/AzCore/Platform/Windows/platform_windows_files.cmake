#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    ../Common/WinAPI/AzCore/std/parallel/internal/condition_variable_WinAPI.h
    ../Common/WinAPI/AzCore/std/parallel/internal/mutex_WinAPI.h
    ../Common/WinAPI/AzCore/std/parallel/internal/semaphore_WinAPI.h
    ../Common/WinAPI/AzCore/std/parallel/internal/thread_WinAPI.cpp
    ../Common/WinAPI/AzCore/std/parallel/internal/thread_WinAPI.h
    AzCore/std/parallel/internal/condition_variable_Platform.h
    AzCore/std/parallel/internal/mutex_Platform.h
    AzCore/std/parallel/internal/semaphore_Platform.h
    AzCore/std/parallel/internal/thread_Platform.h
    AzCore/std/parallel/internal/thread_Windows.cpp
    ../Common/WinAPI/AzCore/std/parallel/config_WinAPI.h
    AzCore/std/parallel/config_Platform.h
    AzCore/std/string/fixed_string_Platform.inl
    ../Common/MSVC/AzCore/std/string/fixed_string_MSVC.inl
    ../Common/VisualStudio/AzCore/Natvis/azcore.natvis
    ../Common/VisualStudio/AzCore/Natvis/azcore.natstepfilter
    ../Common/VisualStudio/AzCore/Natvis/azcore.natjmc
    AzCore/std/time_Windows.cpp
)
