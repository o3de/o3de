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

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

ly_add_target(
    NAME Atom_Feature_Common.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_feature_common_files.cmake
        atom_feature_common_staticlibrary_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${common_source_dir}/${PAL_TRAIT_COMPILER_ID}/atom_feature_common_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            ${pal_source_dir}
        PUBLIC
            Include
            Source
            ../External/ImGui
    COMPILE_DEFINITIONS
        PRIVATE
            IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
        PUBLIC
            Gem::Atom_RPI.Public
            Gem::Atom_Utils.Static
            Gem::Atom_Feature_Common.Public
            Gem::ImGui.imguilib
            3rdParty::lux_core
)

ly_add_target(
    NAME Atom_Feature_Common.Public STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_feature_common_public_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            ${pal_source_dir}
        PUBLIC
            Include
    COMPILE_DEFINITIONS
        PRIVATE
            IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
        PUBLIC
            Gem::Atom_RPI.Public
            Gem::Atom_Utils.Static
)

ly_add_target(
    NAME Atom_Feature_Common ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Atom_Feature_Common.b58e5eed0901428ca78544b04dbd61bd.v0.1.0
    FILES_CMAKE
        atom_feature_common_shared_files.cmake
        ../Assets/atom_feature_common_asset_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_Feature_Common.Static
            Gem::Atom_Feature_Common.Public
            3rdParty::lux_core
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Atom_Feature_Common.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Atom_Feature_Common.Editor.b58e5eed0901428ca78544b04dbd61bd.v0.1.0
        FILES_CMAKE
            atom_feature_common_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                ATOM_FEATURE_COMMON_EDITOR
                ATOM_FEATURE_CORELIGHTS_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                AZ::SceneCore
                Gem::Atom_RPI.Edit
                Gem::Atom_RPI.Public
                Gem::Atom_Utils.Static
                Gem::Atom_Feature_Common.Static
                Gem::Atom_Feature_Common.Public
    )

    ly_add_target(
        NAME Atom_Feature_Common.Builders MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Atom_Feature_Common.Builders.b58e5eed0901428ca78544b04dbd61bd.v0.1.0
        FILES_CMAKE
            atom_feature_common_builders_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source/Builders
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME Atom_Feature_Common.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            atom_feature_common_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
            PUBLIC
                Mocks
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzFramework
                AZ::AzTest
                Gem::Atom_Feature_Common.Static
                Gem::Atom_Feature_Common.Public
    )
    ly_add_googletest(
        NAME Gem::Atom_Feature_Common.Tests
    )
endif()
