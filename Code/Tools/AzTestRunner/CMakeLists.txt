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

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

    include(${pal_dir}/platform_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

    ly_add_target(
        NAME AzTestRunner ${PAL_TRAIT_AZTESTRUNNER_LAUNCHER_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            aztestrunner_files.cmake
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                src
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzTest
                AZ::AzFramework
    )

    ly_add_target(
        NAME AzTestRunner.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            aztestrunner_test_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
    )

    ly_add_googletest(
        NAME AZ::AzTestRunner.Tests
    )

endif()
