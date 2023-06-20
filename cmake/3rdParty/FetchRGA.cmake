#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (APPLE)
    return()  # RGA is not applicable to APPLE host platforms.
endif()

# This script encapsulates fetching RGA as a dependency and providing the
# requested targets. To use it, simply include this script from the CMakeLists.txt
# file of the target platform.

include(FetchContent)

# note that this won't stop it from trying each time, but will stop it from doing anything multiple times in the same
# cmake invocation.
FetchContent_GetProperties(rga)
if (NOT rga_POPULATED) 
    message(STATUS "Checking to see if Radeon(™) GPU Analyzer needs to be downloaded... ")
    if (WIN32)
        FetchContent_Declare(
            RGA
            URL https://github.com/GPUOpen-Tools/radeon_gpu_analyzer/releases/download/2.6.2/rga-windows-x64-2.6.2.zip
            URL_HASH SHA256=35247f29bc81cd86e935b29af26a72cb5f762d4faba2b6aad404f661e639faee
        )
    else()
        FetchContent_Declare(
            RGA
            URL https://github.com/GPUOpen-Tools/radeon_gpu_analyzer/releases/download/2.6.2/rga-linux-2.6.2.tgz
            URL_HASH SHA256=e42e51a12de5ff908785603d9fc5bf88fb8f59dd98f7ed728e8346cae251f712
        )
    endif()

    FetchContent_MakeAvailable(RGA)
    message(STATUS "Radeon(™) GPU Analyzer is located in ${rga_SOURCE_DIR}")
endif()
