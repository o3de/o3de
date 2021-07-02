#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

include(Platform/${PAL_PLATFORM_NAME}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
if (NOT PAL_TRAIT_BUILD_CRCFIX)
    return()
endif()

ly_add_target(
    NAME Crcfix EXECUTABLE
    NAMESPACE AZ
    FILES_CMAKE
        crcfix_files.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)

ly_add_source_properties(
    SOURCES crcfix.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES _CRT_SECURE_NO_WARNINGS
)
