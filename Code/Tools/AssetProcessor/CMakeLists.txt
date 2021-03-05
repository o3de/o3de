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

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

# Builders need to be defined first because we collect the builders and pass them
# to AssetBuilder and AssetProcessor so it loads them.
add_subdirectory(AssetBuilderSDK)
add_subdirectory(AssetBuilder)

include(${CMAKE_CURRENT_SOURCE_DIR}/Platform/${PAL_PLATFORM_NAME}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

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
            3rdParty::XXHash
            AZ::AzCore
            AZ::AzFramework
            AZ::AzQtComponents
            AZ::AzToolsFramework
            AZ::AssetBuilderSDK
            ${additional_dependencies}
    RUNTIME_DEPENDENCIES
        AZ::AssetBuilder
        Legacy::RC
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
    NAME AssetProcessor ${PAL_TRAIT_BUILD_ASSETPROCESSOR_APPLICATION_TYPE}
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
ly_add_source_properties(
    SOURCES native/utilities/BatchApplicationManager.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES LY_METRICS_BUILD_TIME=${LY_METRICS_BUILD_TIME}
)

# Adds the AssetProcessor target as a C preprocessor define so that it can be used as a Settings Registry
# specialization in order to look up the generated .setreg which contains the dependencies
# specified for the target.
if(TARGET AssetProcessor)
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

# Adds the AssetProcessorBatch target as a C preprocessor define so that it can be used as a Settings Registry
# specialization in order to look up the generated .setreg which contains the dependencies
# specified for the target.
if(TARGET AssetProcessorBatch)
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
        TEST_COMMAND $<TARGET_FILE:AZ::AssetProcessor.Tests> --unittest
    )


endif()
