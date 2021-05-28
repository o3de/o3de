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

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME FbxSceneBuilder.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        fbxscenebuilder_files.cmake
        Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            FBX_SCENE_BUILDER_EXPORTS
    INCLUDE_DIRECTORIES
        PUBLIC
            ../..
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
        PUBLIC
            AZ::AzToolsFramework
            AZ::SceneCore
            AZ::SceneData
            AZ::SDKWrapper
)

ly_add_target(
    NAME FbxSceneBuilder MODULE
    NAMESPACE AZ
    FILES_CMAKE
        fbxscenebuilder_shared_files.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            FBX_SCENE_BUILDER_EXPORTS
    INCLUDE_DIRECTORIES
        PUBLIC
            ../..
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::FbxSceneBuilder.Static
        PRIVATE
            AZ::AzCore
)

ly_add_dependencies(AssetBuilder AZ::FbxSceneBuilder)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME FbxSceneBuilder.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            fbxscenebuilder_testing_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::FbxSceneBuilder
    )
    ly_add_googletest(
        NAME AZ::FbxSceneBuilder.Tests
    )
endif()
