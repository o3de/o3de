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
    NAME SceneCore SHARED
    NAMESPACE AZ
    FILES_CMAKE
        scenecore_files.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            SCENE_CORE_EXPORTS
    INCLUDE_DIRECTORIES
        PUBLIC
            ../..
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
            AZ::GFxFramework
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME SceneCore.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            scenecore_testing_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::SceneCore
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::GFxFramework
    )
    ly_add_googletest(
        NAME AZ::SceneCore.Tests
    )
endif()
