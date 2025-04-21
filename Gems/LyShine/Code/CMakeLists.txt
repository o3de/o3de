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
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        lyshine_static_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::LmbrCentral
        PUBLIC
            Gem::Atom_RPI.Public
            Gem::Atom_Utils.Static
            Gem::CommonFeaturesAtom.Static
            Gem::Atom_Bootstrap.Headers
            Gem::AtomFont
            Gem::TextureAtlas
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        lyshine_common_module_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Static
            Legacy::CryCommon
            Gem::LmbrCentral.API
        PUBLIC
            Gem::Atom_RHI.Reflect
            Gem::Atom_RPI.Public
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
        Gem::TextureAtlas
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/LyShineModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# by default, load the above "Gem::${gem_name}" module in Client and Server applications:
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})
# create an alias for other gems to depend on an ${gem_name} public API (may be converted to a target in the future):
ly_create_alias(NAME ${gem_name}.Clients.API NAMESPACE Gem TARGETS Gem::${gem_name}.Clients)
ly_create_alias(NAME ${gem_name}.Servers.API NAMESPACE Gem TARGETS Gem::${gem_name}.Servers)
ly_create_alias(NAME ${gem_name}.Unified.API NAMESPACE Gem TARGETS Gem::${gem_name}.Unified)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Tools.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        AUTORCC
        FILES_CMAKE
            lyshine_uicanvaseditor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                Editor
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Core
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
                AZ::AzCore
                AZ::AzToolsFramework
                Legacy::EditorCommon
                Legacy::EditorCore
                Gem::${gem_name}.Static
                Legacy::CryCommon
                Gem::LmbrCentral.Editor.Static
                Gem::TextureAtlas.Static
                Gem::AtomToolsFramework.Static
                ${additional_dependencies}
            PUBLIC
                Gem::Atom_RPI.Public
                Gem::Atom_Utils.Static
                Gem::CommonFeaturesAtom.Static
                Gem::Atom_Bootstrap.Headers
    )

    ly_add_target(
        NAME ${gem_name}.Tools GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            lyshine_common_module_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                LYSHINE_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                Editor
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommon
                AZ::AzToolsFramework
                Gem::${gem_name}.Tools.Static
                Gem::LmbrCentral.Editor.Static
                Gem::TextureAtlas.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
            Gem::TextureAtlas.Editor
            Gem::UiBasics.Tools
    )

    # by naming this target ${gem_name}.Builders it ensures that it is loaded
    # in any pipeline tools (Like Asset Processor, AssetBuilder, etc)
    ly_add_target(
        NAME ${gem_name}.Builders.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            lyshine_editor_builder_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                Gem::${gem_name}.Static
                Legacy::CryCommon
                Gem::LmbrCentral.Editor
                Gem::TextureAtlas.Editor
                Gem::AtomToolsFramework.Static
                Gem::AtomToolsFramework.Editor
                ${additional_dependencies}
            PUBLIC
                Gem::Atom_RPI.Public
                Gem::Atom_Utils.Static
                Gem::CommonFeaturesAtom.Static
                Gem::Atom_Bootstrap.Headers
    )

    ly_add_target(
        NAME ${gem_name}.Builders GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            lyshine_common_module_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                LYSHINE_BUILDER
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                Editor
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommon
                AZ::AssetBuilderSDK
                Gem::${gem_name}.Builders.Static
                Gem::LmbrCentral.Editor.Static
                Gem::TextureAtlas.Static
        RUNTIME_DEPENDENCIES
            Gem::UiBasics.Builders
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            lyshine_common_module_files.cmake
            lyshine_tests_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                LYSHINE_TESTS
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::${gem_name}.Static
                Legacy::CryCommon
                Gem::LmbrCentral.API
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral
            Gem::TextureAtlas
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
        LABELS REQUIRES_tiaf
    )

    if (PAL_TRAIT_BUILD_HOST_TOOLS)

        ly_add_target(
            NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                lyshine_common_module_files.cmake
                lyshine_editor_tests_files.cmake
            COMPILE_DEFINITIONS
                PRIVATE
                    LYSHINE_EDITOR
                    LYSHINE_BUILDER
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    Editor
                    Pipeline
                    .
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Legacy::CryCommon
                    AZ::AssetBuilderSDK
                    AZ::AzToolsFrameworkTestCommon
                    Gem::${gem_name}.Tools.Static
                    Gem::${gem_name}.Builders.Static
                    Gem::LmbrCentral.Editor.Static
                    Gem::TextureAtlas.Static
            RUNTIME_DEPENDENCIES
                Gem::LmbrCentral.Editor
                Gem::TextureAtlas.Editor
        )
        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
            LABELS REQUIRES_tiaf
        )
    endif()

endif()
