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

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

ly_add_target(
    NAME AzToolsFramework STATIC
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        AzToolsFramework/aztoolsframework_files.cmake
        AzToolsFramework/aztoolsframework_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            AzToolsFramework
            Platform/${PAL_PLATFORM_NAME}
    COMPILE_DEFINITIONS
        PRIVATE
            $<$<CONFIG:debug>:ENABLE_UNDOCACHE_CONSISTENCY_CHECKS>
            O3DE_PYTHON_SITE_PACKAGE_SUBPATH="${LY_PYTHON_VENV_SITE_PACKAGES}"
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::SQLite
            AZ::AzCore
        PUBLIC
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
            AZ::AzFramework
            AZ::AzFramework.NativeUI
            AZ::AzQtComponents
)


################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME AzToolsFrameworkTestCommon STATIC
        NAMESPACE AZ
        AUTOMOC
        FILES_CMAKE
            AzToolsFramework/aztoolsframeworktestcommon_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzTestShared
            PRIVATE
                3rdParty::Qt::Test
                3rdParty::GoogleBenchmark
                AZ::AzToolsFramework
                AZ::AzTest
    )

    ly_add_target(
        NAME AzToolsFramework.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        AUTOMOC
        FILES_CMAKE
            Tests/aztoolsframeworktests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        COMPILE_DEFINITIONS
            PRIVATE
                O3DE_PYTHON_SITE_PACKAGE_SUBPATH="${LY_PYTHON_VENV_SITE_PACKAGES}"
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzTestShared
            PRIVATE
                3rdParty::Qt::Test
                AZ::AzFrameworkTestShared
                AZ::AzToolsFramework
                AZ::AzToolsFrameworkTestCommon
                AZ::AzManipulatorTestFramework.Static
                AZ::AzCoreTestCommon
                AZ::AzTest
                AZ::AzQtComponents
        RUNTIME_DEPENDENCIES
            3rdParty::Qt::Test
    )
    ly_add_googletest(
        NAME AZ::AzToolsFramework.Tests
        LABELS REQUIRES_tiaf
    )
    ly_add_googlebenchmark(
        NAME AZ::AzToolsFramework.Benchmarks
        TARGET AZ::AzToolsFramework.Tests
    )
endif()
