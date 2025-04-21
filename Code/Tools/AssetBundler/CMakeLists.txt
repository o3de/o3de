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

if(TARGET AssetBundlerBatch)
    # Adds a specialized .setreg to identify gems enabled in the active project.
    # This associates the AssetBundlerBatch target with the .Builders gem variants.
    ly_set_gem_variant_to_load(TARGETS AssetBundlerBatch VARIANTS Builders)

    set_source_files_properties(
        source/utils/AssetBundlerBatchBuildTarget.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                O3DE_CMAKE_TARGET="AssetBundlerBatch"
    )
else()
    message(FATAL_ERROR "Cannot set O3DE_CMAKE_TARGET define to AssetBundlerBatch as the target doesn't exist anymore."
        " Perhaps it has been renamed")
endif()


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

if(TARGET AssetBundler)
    # Adds a specialized .setreg to identify gems enabled in the active project.
    # This associates the AssetBundler target with the .Builders gem variants.
    ly_set_gem_variant_to_load(TARGETS AssetBundler VARIANTS Builders)

    set_source_files_properties(
        source/utils/AssetBundlerBuildTarget.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                O3DE_CMAKE_TARGET="AssetBundler"
    )
else()
    message(FATAL_ERROR "Cannot set O3DE_CMAKE_TARGET define to AssetBundler as the target doesn't exist anymore."
        " Perhaps it has been renamed")
endif()


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

    # Copy test files needed for asset processor test to the executable directory
    # This allows the test to reference its files using a portable path that can be provided
    # to users on other machines
    ly_add_target_files(
        TARGETS
            AssetBundler.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/AssetProcessorPlatformConfig.setreg
        OUTPUT_SUBDIRECTORY
            AssetBundler.Tests.dir
    )
    ly_add_target_files(
        TARGETS
            AssetBundler.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/Assets/Engine/SeedAssetList.seed
        OUTPUT_SUBDIRECTORY
            AssetBundler.Tests.dir/Assets/Engine
    )

    ly_add_target_files(
        TARGETS
            AssetBundler.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/DummyProject/AssetProcessorGamePlatformConfig.setreg
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/DummyProject/project.json
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/DummyProject/SeedAssetList.seed
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/DummyProject/SeedAssetListObjectStreamXML.seed
        OUTPUT_SUBDIRECTORY
            AssetBundler.Tests.dir/DummyProject
    )

     ly_add_target_files(
        TARGETS
            AssetBundler.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/Gems/GemA/AssetProcessorGemConfig.setreg
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/Gems/GemA/gem.json
        OUTPUT_SUBDIRECTORY
            AssetBundler.Tests.dir/Gems/GemA
    )

    ly_add_target_files(
        TARGETS
            AssetBundler.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/Gems/GemA/Assets/seedList.seed
        OUTPUT_SUBDIRECTORY
            AssetBundler.Tests.dir/Gems/GemA/Assets
    )

    ly_add_target_files(
        TARGETS
            AssetBundler.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/Gems/GemB/gem.json
        OUTPUT_SUBDIRECTORY
            AssetBundler.Tests.dir/Gems/GemB
    )

    ly_add_target_files(
        TARGETS
            AssetBundler.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/Gems/GemB/Assets/seedList.seed
        OUTPUT_SUBDIRECTORY
            AssetBundler.Tests.dir/Gems/GemB/Assets
    )

    ly_add_target_files(
        TARGETS
            AssetBundler.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/Gems/GemB/Assets/Platforms/GemBSharedAssetList.seed
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/Gems/GemB/Assets/Platforms/randomfile.txt
        OUTPUT_SUBDIRECTORY
            AssetBundler.Tests.dir/Gems/GemB/Assets/Platforms
    )

    ly_add_target_files(
        TARGETS
            AssetBundler.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/Gems/GemB/Assets/Platforms/ios/AssetSeedList.seed
        OUTPUT_SUBDIRECTORY
            AssetBundler.Tests.dir/Gems/GemB/Assets/Platforms/ios
    )

     ly_add_target_files(
        TARGETS
            AssetBundler.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/Gems/GemC/gem.json
        OUTPUT_SUBDIRECTORY
            AssetBundler.Tests.dir/Gems/GemC
    )

endif()
