#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

if(PAL_HOST_PLATFORM_NAME_LOWERCASE STREQUAL "windows")
ly_add_target(
    NAME DeltaCataloger EXECUTABLE
    NAMESPACE AZ
    FILES_CMAKE
        deltacataloger_files.cmake
        deltacataloger_win_files.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
)
else()
ly_add_target(
    NAME DeltaCataloger EXECUTABLE
    NAMESPACE AZ
    FILES_CMAKE
        deltacataloger_files.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
)
endif()

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME DeltaCataloger.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            deltacataloger_test_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzTest
                AZ::AzToolsFramework
                AZ::AzToolsFrameworkTestCommon
    )

    ly_add_googletest(
        NAME AZ::DeltaCataloger.Tests
        LABELS REQUIRES_tiaf
    )

endif()
