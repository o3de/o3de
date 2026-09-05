#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
set(MESHOPTIMIZER_TARGET meshoptimizer)

if (TARGET 3rdParty::${MESHOPTIMIZER_TARGET})
    return()
endif()

set(MESHOPTIMIZER_GIT_REPO "https://github.com/zeux/meshoptimizer.git")
set(MESHOPTIMIZER_GIT_TAG "v1.2")

message(STATUS "Atom Gem uses ${MESHOPTIMIZER_TARGET}-${MESHOPTIMIZER_GIT_TAG} (MIT License) ${MESHOPTIMIZER_GIT_REPO}")

set(BASE_LIBRARY_FOLDER "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")

add_library(${MESHOPTIMIZER_TARGET} STATIC IMPORTED GLOBAL)
add_library(3rdParty::${MESHOPTIMIZER_TARGET} ALIAS ${MESHOPTIMIZER_TARGET})
set_target_properties(${MESHOPTIMIZER_TARGET} PROPERTIES   # Imported location does not support genex https://gitlab.kitware.com/cmake/cmake/-/work_items/22958
    IMPORTED_LOCATION         "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}meshoptimizer${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_DEBUG   "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}meshoptimizer${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE "${BASE_LIBRARY_FOLDER}/release/${CMAKE_STATIC_LIBRARY_PREFIX}meshoptimizer${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

set(meshoptimizer_FOUND TRUE)
