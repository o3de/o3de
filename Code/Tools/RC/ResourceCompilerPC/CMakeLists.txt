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
    NAME ResourceCompilerPC.Static STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        resourcecompilerpc_files.cmake
    PLATFORM_INCLUDE_FILES
        Platform/Common/${PAL_TRAIT_COMPILER_ID}/resourcecompilerpc_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            PCH
    COMPILE_DEFINITIONS
        PRIVATE
            RESOURCE_COMPILER
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::mikkelsen
            Legacy::EditorCommon.Headers
            Legacy::Cry3DEngine.Static
            Legacy::Cry3DEngine.MeshCompiler.Static
            Legacy::Cry3DEngine.CGF.RC.Static
            Legacy::CryCommon.EngineSettings.RC.Static
            Legacy::CryCommonTools
            AZ::SceneData
            AZ::AzToolsFramework
            AZ::AssetBuilderSDK
        PUBLIC
            Legacy::CryCommon
            Legacy::ResourceCompiler.Static
            AZ::AzCore
            ${additional_dependencies}
    RUNTIME_DEPENDENCIES
        Legacy::CryXML
)

ly_add_target(
    NAME ResourceCompilerPC MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY rc_plugins
    FILES_CMAKE
        resourcecompilerpc_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::ResourceCompilerPC.Static
            Legacy::Cry3DEngine.Static
            Legacy::CryCommonTools
            AZ::AzToolsFramework
)

ly_add_dependencies(RC Legacy::ResourceCompilerPC)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ResourceCompilerPC.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            resourcecompilerpc_test_files.cmake
        PLATFORM_INCLUDE_FILES
            Platform/Common/${PAL_TRAIT_COMPILER_ID}/resourcecompilerpc_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                RESOURCE_COMPILER
                CGF_PHYSX_COMPILER
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::ResourceCompilerPC.Static
                AZ::AssetBuilderSDK
                Legacy::CryCommonTools
                Legacy::Cry3DEngine.Static
    )
    ly_add_googletest(
        NAME Legacy::ResourceCompilerPC.Tests
    )
endif()
