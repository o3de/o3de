#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    )

    ly_add_googletest(
        NAME AZ::DeltaCataloger.Tests
    )

endif()
