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
    NAME AssetMemoryAnalyzer.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        assetmemoryanalyzer_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Legacy::CryCommon
            Gem::ImGui.Static
)

ly_add_target(
    NAME AssetMemoryAnalyzer ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        assetmemoryanalyzer_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AssetMemoryAnalyzer.Static
    RUNTIME_DEPENDENCIES
        Gem::ImGui
)

# AssetMemoryAnalyzer is available in clients and servers.
ly_create_alias(NAME AssetMemoryAnalyzer.Clients NAMESPACE Gem TARGETS Gem::AssetMemoryAnalyzer)
ly_create_alias(NAME AssetMemoryAnalyzer.Servers NAMESPACE Gem TARGETS Gem::AssetMemoryAnalyzer)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME AssetMemoryAnalyzer.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            assetmemoryanalyzer_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::AssetMemoryAnalyzer.Static
    )
    ly_add_googletest(
        NAME Gem::AssetMemoryAnalyzer.Tests
    )
endif()

