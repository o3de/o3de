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

ly_add_target(
    NAME SceneProcessing ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        sceneprocessing_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME SceneProcessing.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            sceneprocessing_editor_static_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                .
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::Qt::Widgets
                3rdParty::mikkelsen
                AZ::AzCore
                AZ::SceneCore
                AZ::SceneData
                AZ::AzFramework
                AZ::FbxSceneBuilder
                AZ::AssetBuilderSDK
    )

    ly_add_target(
        NAME SceneProcessing.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            sceneprocessing_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                .
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::SceneProcessing.Editor.Static
        RUNTIME_DEPENDENCIES
            AZ::SceneCore
            AZ::SceneData
    )
    # the SceneProcessing.Editor module above is only used in Builders and Tools.
    ly_create_alias(NAME SceneProcessing.Builders NAMESPACE Gem TARGETS Gem::SceneProcessing.Editor)
    ly_create_alias(NAME SceneProcessing.Tools    NAMESPACE Gem TARGETS Gem::SceneProcessing.Editor)

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME SceneProcessing.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            sceneprocessing_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::SceneProcessing
    )
    ly_add_googletest(
        NAME Gem::SceneProcessing.Tests
    )

    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME SceneProcessing.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                sceneprocessing_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    Gem::SceneProcessing.Editor.Static
                    AZ::AzTest
        )
        ly_add_googletest(
            NAME Gem::SceneProcessing.Editor.Tests
        )
    endif()

endif()
