#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_BUILD_RENDERDOC_ENABLED FALSE)
set(LY_RENDERDOC_PATH "/usr/local" CACHE PATH "Path to RenderDoc.")

# Normalize file path
file(TO_CMAKE_PATH "${LY_RENDERDOC_PATH}" LY_RENDERDOC_PATH)

if(EXISTS "${LY_RENDERDOC_PATH}/librenderdoc.so")
    set(RENDERDOC_RUNTIME_DEPENDENCIES "${RENDERDOC_PATH}/librenderdoc.so")
    set(PAL_TRAIT_BUILD_RENDERDOC_ENABLED TRUE)
endif()
