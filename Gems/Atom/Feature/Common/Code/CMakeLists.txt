#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
set(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

ly_add_target(
    NAME Atom_Feature_Common.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_feature_common_files.cmake
        atom_feature_common_staticlibrary_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            ${pal_source_dir}
        PUBLIC
            Include
            Source
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
            3rdParty::TIFF
    RUNTIME_DEPENDENCIES
        Gem::ImGui.imguilib
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
            3rdParty/ACES
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
    FILES_CMAKE
        atom_feature_common_shared_files.cmake
        ../Assets/atom_feature_common_asset_files.cmake
        ../Editor/atom_feature_common_editor_script_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/runtime_dependencies_clients.cmake
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
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    set(runtime_dependencies_tools ${pal_source_dir}/runtime_dependencies_tools.cmake)
    foreach(pal_tools_platform ${LY_PAL_TOOLS_ENABLED})
        ly_get_list_relative_pal_filename(pal_runtime_dependencies_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${pal_tools_platform})
        list(APPEND runtime_dependencies_tools ${pal_runtime_dependencies_source_dir}/runtime_dependencies_tools.cmake)
    endforeach()

    ly_add_target(
        NAME Atom_Feature_Common.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            atom_feature_common_editor_files.cmake
        PLATFORM_INCLUDE_FILES
            ${runtime_dependencies_tools}
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
        NAME Atom_Feature_Common.Builders GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            atom_feature_common_builders_files.cmake
        PLATFORM_INCLUDE_FILES
            ${runtime_dependencies_tools}
        INCLUDE_DIRECTORIES
            PRIVATE
                Source/Builders
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
        RUNTIME_DEPENDENCIES
            Gem::Atom_RHI.Private
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
