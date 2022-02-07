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

# Builders need to be defined first because we collect the builders and pass them
# to AssetBuilder and AssetProcessor so it loads them.
add_subdirectory(AssetBuilderSDK)
add_subdirectory(AssetBuilder)

ly_add_target(
    NAME AssetProcessor.Static STATIC
    NAMESPACE AZ
    AUTOMOC
    AUTORCC
    FILES_CMAKE
        assetprocessor_static_files.cmake
        Platform/${PAL_PLATFORM_NAME}/assetprocessor_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            Platform/${PAL_PLATFORM_NAME}
        PRIVATE
            native
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Network
            3rdParty::RapidJSON
            3rdParty::SQLite
            3rdParty::xxhash
            AZ::AzCore
            AZ::AzFramework
            AZ::AzQtComponents
            AZ::AzToolsFramework
            AZ::AssetBuilderSDK
            ${additional_dependencies}
    RUNTIME_DEPENDENCIES
        AZ::AssetBuilder
)

# Aggregates all combined AssetBuilders into a single LY_ASSET_BUILDERS #define
get_property(asset_builders GLOBAL PROPERTY LY_ASSET_BUILDERS)
string (REPLACE ";" "," asset_builders "${asset_builders}")
ly_add_source_properties(
    SOURCES native/utilities/ApplicationManager.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES LY_ASSET_BUILDERS="${asset_builders}"
)

ly_add_target(
    NAME AssetProcessor APPLICATION
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        assetprocessor_gui_files.cmake
        Platform/${PAL_PLATFORM_NAME}/assetprocessor_gui_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        Platform/${PAL_PLATFORM_NAME}/assetprocessor_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
        PRIVATE
            native
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AssetProcessor.Static
)

# Adds the AssetProcessor target as a C preprocessor define so that it can be used as a Settings Registry
# specialization in order to look up the generated .setreg which contains the dependencies
# specified for the target.
if(TARGET AssetProcessor)
    ly_set_gem_variant_to_load(TARGETS AssetProcessor VARIANTS Builders)
    set_source_files_properties(
        native/AssetProcessorBuildTarget.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                LY_CMAKE_TARGET="AssetProcessor"
    )
else()
    message(FATAL_ERROR "Cannot set LY_CMAKE_TARGET define to AssetProcessor as the target doesn't exist anymore."
        " Perhaps it has been renamed")
endif()

ly_add_target(
    NAME AssetProcessorBatch.Static STATIC
    NAMESPACE AZ
    AUTOMOC
    FILES_CMAKE
        assetprocessor_static_batch_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
        PRIVATE
            native
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AssetProcessor.Static
)
ly_add_target(
    NAME AssetProcessorBatch EXECUTABLE
    NAMESPACE AZ
    AUTOMOC
    FILES_CMAKE
        assetprocessor_batch_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            native
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AssetProcessorBatch.Static
)

if(LY_DEFAULT_PROJECT_PATH)
    set_property(TARGET AssetProcessor AssetProcessorBatch APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "--project-path=\"${LY_DEFAULT_PROJECT_PATH}\"")
endif()

# Adds the AssetProcessorBatch target as a C preprocessor define so that it can be used as a Settings Registry
# specialization in order to look up the generated .setreg which contains the dependencies
# specified for the target.
if(TARGET AssetProcessorBatch)
    ly_set_gem_variant_to_load(TARGETS AssetProcessorBatch VARIANTS Builders)
    set_source_files_properties(
        native/AssetProcessorBatchBuildTarget.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                LY_CMAKE_TARGET="AssetProcessorBatch"
    )
else()
    message(FATAL_ERROR "Cannot set LY_CMAKE_TARGET define to AssetProcessorBatch as the target doesn't exist anymore."
        " Perhaps it has been renamed")
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    
    ly_add_target(
        NAME AssetProcessor.Tests EXECUTABLE
        NAMESPACE AZ
        AUTOMOC
        AUTORCC
        FILES_CMAKE
            assetprocessor_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                native
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AssetProcessorBatch.Static
                AZ::AzToolsFrameworkTestCommon
    )
    ly_add_source_properties(
        SOURCES native/tests/assetBuilderSDK/assetBuilderSDKTest.cpp
        PROPERTY COMPILE_DEFINITIONS 
        VALUES ${LY_PAL_TOOLS_DEFINES}
    )
    ly_add_source_properties(
        SOURCES native/unittests/AssetProcessorManagerUnitTests.cpp
        PROPERTY COMPILE_DEFINITIONS 
        VALUES LY_CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}"
    )

    ly_add_target_files(
        TARGETS
            AssetProcessor.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/testdata/config_broken_badplatform/AssetProcessorPlatformConfig.setreg
        OUTPUT_SUBDIRECTORY
            testdata/config_broken_badplatform
        )
    ly_add_target_files(
        TARGETS
            AssetProcessor.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/testdata/config_broken_noscans/AssetProcessorPlatformConfig.setreg
        OUTPUT_SUBDIRECTORY
            testdata/config_broken_noscans
    )
    ly_add_target_files(
        TARGETS
            AssetProcessor.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/testdata/config_broken_recognizers/AssetProcessorPlatformConfig.setreg
        OUTPUT_SUBDIRECTORY
            testdata/config_broken_recognizers
    )
    ly_add_target_files(
        TARGETS
            AssetProcessor.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/testdata/config_broken_noplatform/AssetProcessorPlatformConfig.setreg
        OUTPUT_SUBDIRECTORY
            testdata/config_broken_noplatform
    )
    ly_add_target_files(
        TARGETS
            AssetProcessor.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/testdata/config_regular/AssetProcessorPlatformConfig.setreg
        OUTPUT_SUBDIRECTORY
            testdata/config_regular
    )
    ly_add_target_files(
        TARGETS
            AssetProcessor.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/testdata/config_regular_platform_scanfolder/AssetProcessorPlatformConfig.setreg
        OUTPUT_SUBDIRECTORY
            testdata/config_regular_platform_scanfolder
    )
    ly_add_target_files(
        TARGETS
            AssetProcessor.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/testdata/EmptyDummyProject/AssetProcessorGamePlatformConfig.setreg
        OUTPUT_SUBDIRECTORY
            testdata/EmptyDummyProject
    )
    ly_add_target_files(
        TARGETS
            AssetProcessor.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/testdata/DummyProject/AssetProcessorGamePlatformConfig.setreg
        OUTPUT_SUBDIRECTORY
            testdata/DummyProject
    )
    ly_add_target_files(
        TARGETS
            AssetProcessor.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/testdata/config_metadata/AssetProcessorPlatformConfig.setreg
        OUTPUT_SUBDIRECTORY
            testdata/config_metadata
    )

    # Have the AssetProcessorTest use the LY_CMAKE_TARGET define of AssetProcessorBatch for the purpose
    # of looking up the generated cmake build dependencies settings registry .setreg file
    # It is tied to the UnitTestRunner.cpp file
    if(TARGET AssetProcessorBatch)
        set_source_files_properties(
            native/unittests/UnitTestRunner.cpp
            PROPERTIES
                COMPILE_DEFINITIONS
                    LY_CMAKE_TARGET="AssetProcessorBatch"
        )
    else()
        message(FATAL_ERROR "Cannot set LY_CMAKE_TARGET define to AssetProcessorBatch as the target doesn't exist anymore."
            " Perhaps it has been renamed")
    endif()

    ly_add_googletest(
        NAME AZ::AssetProcessor.Tests
        TEST_COMMAND $<TARGET_FILE:AZ::AssetProcessor.Tests> --unittest --gtest_filter=-*.SUITE_sandbox*
    )

endif()
