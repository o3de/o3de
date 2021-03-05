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

ly_add_target(
    NAME ResourceCompilerScene.Static STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        resourcecompilerscene_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            ../..
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Legacy::CryCommon
            Legacy::ResourceCompiler.Static
            Legacy::ResourceCompilerPC.Static
            AZ::AssetBuilderSDK
        PUBLIC
            AZ::AzToolsFramework
            AZ::SceneCore
            AZ::FbxSceneBuilder
            AZ::GFxFramework
)


if(TARGET AssetBuilder)
    # The SceneBuilder uses the AssetBuilder gem dependency list to determine which gems it should load in order to process
    # Therefore the AssetBuilder target is set as the LY_CMAKE_TARGET for the SceneCompiler.cpp in order
    # to allow it to use the Settings Regisry specialization the AssetBuilder in order
    # to load the cmake_dependencies.<project>.AssetBuilder.setreg file to gather the list of gems to load
    set_source_files_properties(
        SceneCompiler.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                LY_CMAKE_TARGET="AssetBuilder"
    )
else()
    message(FATAL_ERROR "Cannot set LY_CMAKE_TARGET define to AssetBuilder as the AssetBuilder TARGET doesn't exist.")
endif()

ly_add_target(
    NAME ResourceCompilerScene MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY rc_plugins
    FILES_CMAKE
        resourcecompilerscene_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            ../..
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::ResourceCompilerScene.Static
            Legacy::ResourceCompiler.Static
            Legacy::CryCommon
            Gem::EditorPythonBindings.Static
)

ly_add_dependencies(RC Legacy::ResourceCompilerScene)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ResourceCompilerScene.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            resourcecompilerscene_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::ResourceCompilerScene.Static
                Legacy::CryCommon
                Legacy::ResourceCompiler.Static
        RUNTIME_DEPENDENCIES
            Legacy::CrySystem
    )
    ly_add_googletest(
        NAME Legacy::ResourceCompilerScene.Tests
    )
endif()
