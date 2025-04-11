#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(AzFramework/feature_options.cmake)

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})
set(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)

if(LY_PIX_ENABLED)
    set(AZ_FRAMEWORK_NATIVEUI_PIX_BUILD_DEPENDENCIES 3rdParty::pix)
endif()

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
            AZ::AzNetworking
            3rdParty::zstd
            3rdParty::lz4
)

ly_add_target(
    NAME AzFramework.NativeUI STATIC
    NAMESPACE AZ
    FILES_CMAKE
        AzFramework/azframework_nativeui_files.cmake
        ${pal_dir}/platform_nativeui_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_nativeui_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
        PRIVATE
            ${common_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            ${AZ_FRAMEWORK_NATIVEUI_PIX_BUILD_DEPENDENCIES}
)

set(LY_SEARCH_MODE_DEFINE $<$<BOOL:"${LY_ARCHIVE_FILE_SEARCH_MODE}">:LY_ARCHIVE_FILE_SEARCH_MODE=${LY_ARCHIVE_FILE_SEARCH_MODE}>)

ly_add_source_properties(
    SOURCES
        AzFramework/Archive/ArchiveVars.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES ${LY_SEARCH_MODE_DEFINE})

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    o3de_pal_dir(test_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Tests/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

    ly_add_target(
        NAME AzFrameworkTestShared STATIC
        NAMESPACE AZ
        FILES_CMAKE
            Tests/framework_shared_tests_files.cmake
            AzFramework/Physics/physics_mock_files.cmake
            Tests/terrain_mock_files.cmake
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
                ${test_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    ${test_pal_dir}
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
            LABELS REQUIRES_tiaf;TIAF_shard_fixture
        )

        ly_add_target(
            NAME AzFramework.NativeUI.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE AZ
            FILES_CMAKE
                Tests/framework_nativeui_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    ${test_pal_dir}
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzFramework
                    AZ::AzFramework.NativeUI
                    AZ::AzTest
                    AZ::AzTestShared
                    AZ::AzFrameworkTestShared
            RUNTIME_DEPENDENCIES
                AZ::ProcessLaunchTest
        )
        ly_add_googletest(
            NAME AZ::AzFramework.NativeUI.Tests
        )


        include(${test_pal_dir}/platform_specific_test_targets.cmake)

    endif()

endif()
