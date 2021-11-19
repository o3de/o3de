#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)

ly_add_target(
    NAME AzFramework STATIC
    NAMESPACE AZ
    FILES_CMAKE
        AzFramework/azframework_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
        PRIVATE
            ${common_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
        PUBLIC
            AZ::GridMate
            3rdParty::zstd
            3rdParty::lz4
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Tests/Platform/${PAL_PLATFORM_NAME})

    ly_add_target(
        NAME AzFrameworkTestShared STATIC
        NAMESPACE AZ
        FILES_CMAKE
            Tests/framework_shared_tests_files.cmake
            AzFramework/Physics/physics_mock_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
            PUBLIC
                AZ::AzTest
                AZ::AzTestShared
    )
    
    if(PAL_TRAIT_BUILD_HOST_TOOLS)

        ly_add_target(
            NAME ProcessLaunchTest EXECUTABLE
            NAMESPACE AZ
            FILES_CMAKE
                Tests/process_launch_test_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzCore
                    AZ::AzFramework
        )

        ly_add_target(
            NAME AzFramework.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE AZ
            FILES_CMAKE
                Tests/frameworktests_files.cmake
                ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    ${pal_dir}
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzFramework
                    AZ::AzTest
                    AZ::AzTestShared
                    AZ::AzFrameworkTestShared
            RUNTIME_DEPENDENCIES
                AZ::ProcessLaunchTest
        )
        ly_add_googletest(
            NAME AZ::AzFramework.Tests
        )

        include(${pal_dir}/platform_specific_test_targets.cmake)

    endif()

endif()
