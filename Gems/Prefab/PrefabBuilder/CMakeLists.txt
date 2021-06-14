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
    NAME PrefabBuilder.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        prefabbuilder_files.cmake
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzToolsFramework
            AZ::AssetBuilderSDK
)

ly_add_target(
    NAME PrefabBuilder MODULE
    NAMESPACE Gem
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    FILES_CMAKE
        prefabbuilder_module_files.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::PrefabBuilder.Static
)

# the prefab builder only needs to be active in builders
# use the PrefabBuilder module in Clients and Servers:
ly_create_alias(NAME PrefabBuilder.Builders NAMESPACE Gem TARGETS Gem::PrefabBuilder)

# we automatically add this gem, if it is present, to all our known set of builder applications:
ly_enable_gems(GEMS PrefabBuilder VARIANTS Builders TARGETS AssetProcessor AssetProcessorBatch AssetBuilder)

# if you have a custom builder application in your project, then use ly_enable_gems() to
# add it to that application for your project, like this to make YOUR_TARGET_NAME load it automatically
# ly_enable_gems(PROJECT (YOUR_PROJECT_NAME) GEMS PrefabBuilder VARIANTS Builders TARGETS (YOUR_TARGET_NAME) )

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME PrefabBuilder.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            prefabbuilder_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::PrefabBuilder.Static
    )
    ly_add_googletest(
        NAME Gem::PrefabBuilder.Tests
    )
endif()
