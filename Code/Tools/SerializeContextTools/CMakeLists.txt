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
if (NOT PAL_TRAIT_BUILD_SERIALIZECONTEXTTOOLS)
    return()
endif()

ly_add_target(
    NAME SerializeContextTools EXECUTABLE
    NAMESPACE AZ
    FILES_CMAKE
        serializecontexttools_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
)
