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
set(MESHOPTIMIZER_GIT_TAG "v0.24")

include(FetchContent)
FetchContent_Declare(
        ${MESHOPTIMIZER_TARGET}
        GIT_REPOSITORY ${MESHOPTIMIZER_GIT_REPO}
        GIT_TAG ${MESHOPTIMIZER_GIT_TAG}
)
set(MESHOPT_INSTALL OFF)
FetchContent_MakeAvailable(meshoptimizer)

message(STATUS "Atom Gem uses ${MESHOPTIMIZER_TARGET}-${MESHOPTIMIZER_GIT_TAG} (MIT License) ${MESHOPTIMIZER_GIT_REPO}")

get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
ly_get_engine_relative_source_dir(${this_gem_root} relative_this_gem_root)
set_property(TARGET ${MESHOPTIMIZER_TARGET} PROPERTY FOLDER "${relative_this_gem_root}/External")

add_library(3rdParty::${MESHOPTIMIZER_TARGET} ALIAS ${MESHOPTIMIZER_TARGET})
ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findmeshoptimizer.cmake DESTINATION cmake/3rdParty)

set(meshoptimizer_FOUND TRUE)