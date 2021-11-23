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

include(${CMAKE_CURRENT_SOURCE_DIR}/Platform/${PAL_PLATFORM_NAME}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# AssetBundlerBatch - CLI Application
ly_add_target(
    NAME AssetBundlerBatch.Static STATIC
    NAMESPACE AZ
    AUTOMOC
    FILES_CMAKE
        assetbundlerbatch_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
            AZ::AzToolsFramework
            ${additional_dependencies}
)

ly_add_target(
    NAME AssetBundlerBatch EXECUTABLE
    NAMESPACE AZ
    FILES_CMAKE
        assetbundlerbatch_exe_files.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            AB_BATCH_MODE
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AssetBundlerBatch.Static
)

# Adds a specialized .setreg to identify gems enabled in the active project.
# This associates the AssetBundlerBatch target with the .Builders gem variants.
ly_set_gem_variant_to_load(TARGETS AssetBundlerBatch VARIANTS Builders)

# AssetBundler - Qt GUI Application
ly_add_target(
    NAME AssetBundler ${PAL_TRAIT_BUILD_ASSETBUNDLER_APPLICATION_TYPE}
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE 
        assetbundlergui_files.cmake
        assetbundler_exe_files.cmake
        Platform/${PAL_PLATFORM_NAME}/assetbundlergui_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Platform/${PAL_PLATFORM_NAME}
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
            AZ::AzToolsFramework
            AZ::AssetBundlerBatch.Static
            ${additional_dependencies}
)

if(LY_DEFAULT_PROJECT_PATH)
    set_property(TARGET AssetBundler AssetBundlerBatch APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "--project-path=\"${LY_DEFAULT_PROJECT_PATH}\"")
endif()

# Adds a specialized .setreg to identify gems enabled in the active project.
# This associates the AssetBundler target with the .Builders gem variants.
ly_set_gem_variant_to_load(TARGETS AssetBundler VARIANTS Builders)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME AssetBundler.Tests EXECUTABLE
        NAMESPACE AZ
        FILES_CMAKE
            assetbundlerbatch_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AssetBundlerBatch.Static
                AZ::AzFrameworkTestShared
    )

    ly_add_googletest(
        NAME AZ::AssetBundler.Tests
        TEST_COMMAND $<TARGET_FILE:AZ::AssetBundler.Tests> --unittest
    )

endif()
