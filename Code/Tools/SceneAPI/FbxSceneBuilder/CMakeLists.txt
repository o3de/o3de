#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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
