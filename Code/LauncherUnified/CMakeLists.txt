#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

include(${pal_dir}/LauncherUnified_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

ly_add_target(
    NAME Launcher.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        launcher_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            ${pal_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzGameFramework
            Legacy::CryCommon
)

ly_add_target(
    NAME Launcher.Headless.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        launcher_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            ${pal_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzGameFramework.Headless
            Legacy::CryCommon
)


ly_add_target(
    NAME Launcher.Game.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        launcher_game_files.cmake
        ${pal_dir}/launcher_game_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzGameFramework
            Legacy::CryCommon
)

if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)

    ly_add_target(
        NAME Launcher.Server.Static STATIC
        NAMESPACE AZ
        FILES_CMAKE
            launcher_server_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Legacy::CryCommon
    )

endif()

if(PAL_TRAIT_BUILD_UNIFIED_SUPPORTED)

    ly_add_target(
        NAME Launcher.Unified.Static STATIC
        NAMESPACE AZ
        FILES_CMAKE
            launcher_unified_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzGameFramework
                Legacy::CryCommon
    )

endif()

include(${CMAKE_CURRENT_LIST_DIR}/launcher_generator.cmake)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    
    ly_add_target(
        NAME Launcher.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            launcher_test_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                LY_CMAKE_TARGET="Launcher_Tests"
        INCLUDE_DIRECTORIES
            PRIVATE
                . 
                ${pal_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::Launcher.Static
    )
    ly_add_googletest(
        NAME AZ::Launcher.Tests
        LABELS REQUIRES_tiaf
    )

endif()

################################################################################
# Install
################################################################################

ly_install_files(
    FILES
        launcher_generator.cmake
        launcher_project_files.cmake
        LauncherProject.cpp
        StaticModules.in
    DESTINATION LauncherGenerator
)
ly_install_directory(
    DIRECTORIES 
        Platform/${PAL_PLATFORM_NAME}
        Platform/Common
    DESTINATION LauncherGenerator/Platform
    VERBATIM
)
ly_install_files(
    FILES FindLauncherGenerator.cmake
    DESTINATION cmake
)
