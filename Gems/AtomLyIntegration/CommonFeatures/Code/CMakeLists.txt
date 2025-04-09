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
    NAME ${gem_name}.Public HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        atomlyintegration_commonfeatures_public_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
            Gem::Atom_Feature_Common.Public
            Gem::AtomImGuiTools.Public
)

# Add alias that references the "AtomLyIntegration_CommonFeatures.Public" for backwards compatibility
ly_create_alias(NAME AtomLyIntegration_CommonFeatures.Public NAMESPACE Gem
    TARGETS ${gem_name}.Public)

ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atomlyintegration_commonfeatures_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::${gem_name}.Public
            Gem::LmbrCentral.Static
            Gem::GradientSignal.Static
            Gem::SurfaceData.Static
            Gem::Atom_Feature_Common.Public
            Gem::Atom_Bootstrap.Headers
)

# Add alias that references the "AtomLyIntegration_CommonFeatures.Static" name for backwards compatibility
ly_create_alias(NAME AtomLyIntegration_CommonFeatures.Static NAMESPACE Gem
    TARGETS ${gem_name}.Static)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atomlyintegration_commonfeatures_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Static
    RUNTIME_DEPENDENCIES
        Gem::Atom_RPI.Private
)

# Add alias that references the "AtomLyIntegration_CommonFeatures" name for backwards compatibility
ly_create_alias(NAME AtomLyIntegration_CommonFeatures NAMESPACE Gem
    TARGETS ${gem_name})

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/Module.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# The ${gem_name} module is used for Clients and Servers
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem
    TARGETS
        Gem::${gem_name}
        Gem::GradientSignal.Clients
)
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem
    TARGETS
        Gem::${gem_name}
        Gem::GradientSignal.Servers
)
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem
    TARGETS
        Gem::${gem_name}
        Gem::GradientSignal.Unified
)

# Add aliases that references the legacy "AtomLyIntegration_CommonFeatures" name
# using the real name of the Gem through the ${gem_name} variable
ly_create_alias(NAME Gem::AtomLyIntegration_CommonFeatures.Clients NAMESPACE Gem
    TARGETS
        ${gem_name}
)
ly_create_alias(NAME Gem::AtomLyIntegration_CommonFeatures.Servers NAMESPACE Gem
    TARGETS
        ${gem_name}
)
ly_create_alias(NAME Gem::AtomLyIntegration_CommonFeatures.Unified NAMESPACE Gem
    TARGETS
        ${gem_name}
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor.Static STATIC
        NAMESPACE Gem
        AUTOUIC
        AUTOMOC
        AUTORCC
        FILES_CMAKE
            atomlyintegration_commonfeatures_editor_files.cmake
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                ATOMLYINTEGRATION_FEATURE_COMMON_EDITOR
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::${gem_name}.Static
                Gem::Atom_RPI.Edit
                Gem::AtomToolsFramework.Static
                Gem::AtomToolsFramework.Editor
                AZ::SceneCore
                AZ::SceneData
                Legacy::Editor.Headers
                Legacy::EditorCommon
                Legacy::CryCommon
        RUNTIME_DEPENDENCIES
            3rdParty::OpenImageIO::Tools::Binaries
            3rdParty::OpenImageIO::Tools::PythonPlugins
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            atomlyintegration_commonfeatures_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                ATOMLYINTEGRATION_FEATURE_COMMON_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::${gem_name}.Editor.Static
        RUNTIME_DEPENDENCIES
            Legacy::EditorCommon
    )

    # Add alias that references the "AtomLyIntegration_CommonFeatures.Editor" for backwards compatibility
    ly_create_alias(NAME AtomLyIntegration_CommonFeatures.Editor NAMESPACE Gem
        TARGETS ${gem_name}.Editor)

    # The ${gem_name}.Editor module is used for Builders and Tools
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem
        TARGETS
            Gem::${gem_name}.Editor
            Gem::Atom_Feature_Common.Builders
            Gem::Atom_RPI.Builders
            Gem::GradientSignal.Builders
    )
    ly_create_alias(NAME ${gem_name}.Tools NAMESPACE Gem
        TARGETS
            Gem::${gem_name}.Editor
            Gem::GradientSignal.Tools
    )

    # Add aliases that references the real name of the gem legacy name "AtomLyIntegration_CommonFeatures"
    # Tools and Builders variants using the ${gem_name} variable
    ly_create_alias(NAME Gem::AtomLyIntegration_CommonFeatures.Builders NAMESPACE Gem
        TARGETS
            ${gem_name}
    )
    ly_create_alias(NAME Gem::AtomLyIntegration_CommonFeatures.Tools NAMESPACE Gem
        TARGETS
            ${gem_name}
    )

    ################################################################################
    # Tests
    ################################################################################
    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                atomlyintegration_commonfeatures_editor_test_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzTestShared
                    AZ::AzToolsFramework
                    AZ::AzToolsFrameworkTestCommon
                    Gem::${gem_name}.Static
                    Gem::${gem_name}.Editor.Static
            RUNTIME_DEPENDENCIES
                Gem::LmbrCentral.Editor
        )
        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
            LABELS REQUIRES_tiaf
        )
    endif()
endif()

# AtomLyIntegration_CommonFeatures gem targets are required as part of the Editor and AssetProcessor
# due to the AZ::Render::EditorDirectionalLightComponent, AZ::Render::EditorMeshComponent,
# AZ::Render::EditorGridComponent, AZ::Render::EditorHDRiSkyboxComponent,
# AZ::Render::EditorImageBasedLightComponent being saved as part of the DefaultLevel.prefab
