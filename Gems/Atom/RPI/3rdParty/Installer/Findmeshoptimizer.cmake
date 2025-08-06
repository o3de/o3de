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

message(STATUS "Atom Gem uses ${MESHOPTIMIZER_TARGET}-${MESHOPTIMIZER_GIT_TAG} (MIT License) ${MESHOPTIMIZER_GIT_REPO}")

add_library(${MESHOPTIMIZER_TARGET} IMPORTED INTERFACE GLOBAL)
add_library(3rdParty::${MESHOPTIMIZER_TARGET} ALIAS ${MESHOPTIMIZER_TARGET})

set(meshoptimizer_FOUND TRUE)