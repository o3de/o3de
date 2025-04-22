#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_ATOM_RHI_VULKAN_SUPPORTED TRUE)
set(VULKAN_VALIDATION_LAYER 3rdParty::vulkan-validationlayers)

set(LY_AFTERMATH_PATH "" CACHE PATH "Path to Aftermath.")
if (NOT "${LY_AFTERMATH_PATH}" STREQUAL "")
    set(ATOM_AFTERMATH_PATH_CMAKE_FORMATTED "${LY_AFTERMATH_PATH}")
endif()

set(PAL_TRAIT_AFTERMATH_AVAILABLE FALSE)
unset(aftermath_header CACHE)
if ("${ATOM_AFTERMATH_PATH_CMAKE_FORMATTED}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{ATOM_AFTERMATH_PATH}" ATOM_AFTERMATH_PATH_CMAKE_FORMATTED)
endif()

find_file(aftermath_header
    GFSDK_Aftermath.h
    PATHS
        "${ATOM_AFTERMATH_PATH_CMAKE_FORMATTED}/include"
)  
mark_as_advanced(CLEAR, aftermath_header)
if(aftermath_header)
    set(PAL_TRAIT_AFTERMATH_AVAILABLE TRUE)
endif()
