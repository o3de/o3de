#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME GFxFramework STATIC
        NAMESPACE AZ
        FILES_CMAKE
            gfxframework_files.cmake
        PLATFORM_INCLUDE_FILES
            ../Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                ..
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzToolsFramework
    )
endif()

