#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# File to tweak compiler settings before compiler detection happens (before project() is called)
# We dont have PAL enabled at this point, so we can only use pure-CMake variables

# if we are in Script Only ("Quick Start") Mode, we override the compiler with a do-nothing fake one
# doing so is slightly different on each platform, so it uses a different file each.
get_property(O3DE_SCRIPT_ONLY GLOBAL PROPERTY "O3DE_SCRIPT_ONLY")

if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
    if(O3DE_SCRIPT_ONLY)
        include(${CMAKE_CURRENT_LIST_DIR}/Platform/Linux/Toolchain_scriptonly_linux.cmake)
    else()
        include(${CMAKE_CURRENT_LIST_DIR}/Platform/Linux/CompilerSettings_linux.cmake)
    endif()
elseif("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    if(O3DE_SCRIPT_ONLY)
        include(${CMAKE_CURRENT_LIST_DIR}/Platform/Windows/Toolchain_scriptonly_windows.cmake)
    endif()
elseif("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Darwin")
    if(O3DE_SCRIPT_ONLY)
        message(FATAL_ERROR "Script only (quick start) projects are not yet supported on MacOS")
    endif()
endif()
