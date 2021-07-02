#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

    ly_add_target(
        NAME AzFrameworkTestShared STATIC
        NAMESPACE AZ
        FILES_CMAKE
            framework_shared_tests_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
    )

    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ProcessLaunchTest EXECUTABLE
            NAMESPACE AZ
            FILES_CMAKE
                process_launch_test_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzCore
                    AZ::AzFramework
        )

        ly_add_target(
            NAME Framework.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE AZ
            FILES_CMAKE
                frameworktests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    ${pal_dir}
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzToolsFramework
                    AZ::AzTestShared
                    AZ::AzFrameworkTestShared
            RUNTIME_DEPENDENCIES
                AZ::ProcessLaunchTest
        )
        ly_add_googletest(
            NAME AZ::Framework.Tests
        )
    endif()

endif()
