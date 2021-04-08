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

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)

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