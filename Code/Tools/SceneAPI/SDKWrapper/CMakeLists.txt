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

message(WARNING "Gene ${CMAKE_MODULE_PATH}")

set(ASSIMP_BUILD_ZLIB OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS ON)
set(ASSIMP_BUILD_USD_IMPORTER ON)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(ASSIMP_INSTALL OFF)  # Disable since we're using Assimp as a sub module

get_property(
    compile_options
    DIRECTORY
    PROPERTY COMPILE_OPTIONS
)

include(Platform/${PAL_PLATFORM_NAME}/PAL_assimp_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Temporarily disable warnings as errors from TinyUSDZ (Assimp library dependency)
#set(COMPILE_WARNING_AS_ERROR OFF CACHE BOOL "" FORCE)

# Include compiler settings for Assimp libraries
include(Platform/Common/${PAL_TRAIT_COMPILER_ID}/PAL_assimp_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake)

#set_property(
#    DIRECTORY
#    APPEND
#    PROPERTY COMPILE_DEFINITIONS DT_POLYREF64
#)

# Assimp requires ZLIB
find_package(zlib REQUIRED)

#target_link_libraries(assimp PRIVATE zlib)


FetchContent_MakeAvailable(assimp)

# Restore compile options
set_property(
    DIRECTORY
    PROPERTY COMPILE_OPTIONS ${compile_options}
)
unset(compile_options)

# Reenable warnings as errors
#set(COMPILE_WARNING_AS_ERROR ON CACHE BOOL "" FORCE)


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
