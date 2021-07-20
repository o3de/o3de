#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME PythonAssetBuilder.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        pythonassetbuilder_common_files.cmake
    PLATFORM_INCLUDE_FILES
        Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/pythonassetbuilder_static_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
        PUBLIC
            3rdParty::Python
            Gem::EditorPythonBindings.Static
            AZ::AssetBuilderSDK
            AZ::AzToolsFramework
)

ly_add_target(
    NAME PythonAssetBuilder.Editor GEM_MODULE

    NAMESPACE Gem
    FILES_CMAKE
        pythonassetbuilder_editor_files.cmake
        pythonassetbuilder_shared_files.cmake
    PLATFORM_INCLUDE_FILES
        Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/pythonassetbuilder_static_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::EditorPythonBindings.Static
            AZ::AssetBuilderSDK
    RUNTIME_DEPENDENCIES
        Gem::EditorPythonBindings.Editor
)

# the above target is used in both builders like AssetProcessor and Tools like the Editor
# but is not used in clients or servers
ly_create_alias(NAME PythonAssetBuilder.Tools     NAMESPACE Gem TARGETS Gem::PythonAssetBuilder.Editor)
ly_create_alias(NAME PythonAssetBuilder.Builders  NAMESPACE Gem TARGETS Gem::PythonAssetBuilder.Editor)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME PythonAssetBuilder.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            pythonassetbuilder_tests_files.cmake
        PLATFORM_INCLUDE_FILES
            Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/pythonassetbuilder_tests_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::PythonAssetBuilder.Static
    )
    ly_add_googletest(
        NAME Gem::PythonAssetBuilder.Tests
    )
endif()
