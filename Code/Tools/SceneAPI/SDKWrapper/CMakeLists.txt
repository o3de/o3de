#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

set(sdkwrapper_dir ${CMAKE_CURRENT_LIST_DIR}/../SDKWrapper)

# Add Assimp dependency using CMake's FetchContent
# Note: Assimp requires lowering of the warning level to compile, due to a child dependency of TinyUSDZ
include(FetchContent)
FetchContent_Declare(
    assimp
    GIT_REPOSITORY "https://github.com/assimp/assimp"
    GIT_TAG "d41b6b253d215a003ed6cfd7db197dbe7e7e8206"
)

set(ASSIMP_BUILD_ZLIB OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_USD_IMPORTER ON)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)  # No sure why this doesn't work. Warnings are still treated as error; need to add warning disabled commandline parameters to PAL_assimp_msvc.cmake
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)  # Disable since we're using Assimp as a sub module

# Clear O3DE's CXX_FLAGS before building Assimp, then restore them after 
set(CMAKE_CXX_FLAGS_OLD "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "")

include(Platform/${PAL_PLATFORM_NAME}/PAL_assimp_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Include compiler settings for Assimp libraries
include(Platform/Common/${PAL_TRAIT_COMPILER_ID}/PAL_assimp_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake)

# Assimp requires ZLIB
find_package(zlib REQUIRED)

FetchContent_MakeAvailable(assimp)

# Restore compile options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_OLD}")

# Place external assimp under SDKWrapper target.
get_property(sdkwrapper_target_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
ly_get_engine_relative_source_dir(${sdkwrapper_target_root} relative_sdkwrapper_target_root)
set(assimpLibraries assimp)
foreach(assimpLibrary ${assimpLibraries})
    set_property(TARGET ${assimpLibrary} PROPERTY FOLDER "${relative_sdkwrapper_target_root}/External")
endforeach()


ly_add_target(
    NAME SDKWrapper STATIC
    NAMESPACE AZ
    FILES_CMAKE
        sdkwrapper_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ../..
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzToolsFramework
        PUBLIC
            assimp
)
