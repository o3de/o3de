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


include(FetchContent)
FetchContent_Declare(
    assimp
    GIT_REPOSITORY "https://github.com/assimp/assimp"
    GIT_TAG "d41b6b253d215a003ed6cfd7db197dbe7e7e8206"
    BINARY_DIR ${ASSIMP_BINARY_DIR}
)

# Clear O3DE's CXX_FLAGS before building Assimp, then restore them after 
set(RESTORE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "")

set(ASSIMP_BUILD_ZLIB OFF)  # Don't rebuild ZLIB. O3DE's comes with its own ZLIB 3rdParty package.
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_USD_IMPORTER ON)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)  # Not sure why this doesn't work. Warnings are still treated as error; needed to add warning disabled commandline parameters to PAL_assimp_<compiler>.cmake
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)  # Disable since we're using Assimp as a sub module
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)  # Stop Assimp from adding 'd' to debug target outputs (mylibraryd.dll) https://github.com/assimp/assimp/blob/f63625256c4e6f18fca8e7dc857b47a02320e867/CMakeLists.txt#L411
set(ASSIMP_HUNTER_ENABLED OFF)

# Include compiler settings for Assimp libraries based on compiler
include(Platform/Common/${PAL_TRAIT_COMPILER_ID}/PAL_assimp_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake)

# Assimp requires ZLIB
find_package(ZLIB REQUIRED)

FetchContent_MakeAvailable(assimp)

# Restore compile options
set(CMAKE_CXX_FLAGS "${RESTORE_CXX_FLAGS}")

set(sdkwrapper_dir ${CMAKE_CURRENT_LIST_DIR}/../SDKWrapper)

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


# Move the Assimp projects into the SDKWrapper module folder
get_target_property(SDKWRAPPER_FOLDER SDKWrapper FOLDER)

set(assimpLibraries assimp tinyusdz tinyusdz_static)

# Move UpdateAssimpLibsDebugSymbolsAndDLLs if needed. Not all platforms produce this target.
if(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs)
    list(APPEND assimpLibraries UpdateAssimpLibsDebugSymbolsAndDLLs)
endif()

foreach(assimpLibrary ${assimpLibraries})
    set_property(TARGET ${assimpLibrary} PROPERTY FOLDER "${SDKWRAPPER_FOLDER}/assimp")
endforeach()
