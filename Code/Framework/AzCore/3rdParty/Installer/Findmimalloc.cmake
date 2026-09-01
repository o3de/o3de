#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::mimalloc)
    return()
endif()

set(MIMALLOC_GIT_REPOSITORY "https://github.com/microsoft/mimalloc.git")
set(MIMALLOC_VERSION_STRING "v3.5.1")

message(STATUS "AzCore uses mimalloc ${MIMALLOC_VERSION_STRING} (MIT) from ${MIMALLOC_GIT_REPOSITORY}")

add_library(mimalloc STATIC IMPORTED GLOBAL)

set(BASE_LIBRARY_FOLDER "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")
set_target_properties(mimalloc PROPERTIES 
    IMPORTED_LOCATION         "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}mimalloc${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_DEBUG   "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}mimalloc${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE "${BASE_LIBRARY_FOLDER}/release/${CMAKE_STATIC_LIBRARY_PREFIX}mimalloc${CMAKE_STATIC_LIBRARY_SUFFIX}")
add_library(3rdParty::mimalloc ALIAS mimalloc)

set(mimalloc_FOUND TRUE)
