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
    NAME AtomLyIntegration_CommonFeatures.Public HEADERONLY
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

ly_add_target(
    NAME AtomLyIntegration_CommonFeatures.Static STATIC
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
            Gem::AtomLyIntegration_CommonFeatures.Public
            Gem::LmbrCentral.Static
            Gem::GradientSignal.Static
            Gem::SurfaceData.Static
            Gem::Atom_Feature_Common.Static
            Gem::Atom_Bootstrap.Headers
)

ly_add_target(
    NAME AtomLyIntegration_CommonFeatures ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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
            Gem::AtomLyIntegration_CommonFeatures.Static
    RUNTIME_DEPENDENCIES
        Gem::Atom_RPI.Private
)

# The AtomLyIntegration_CommonFeatures module is used for Clients and Servers
ly_create_alias(NAME AtomLyIntegration_CommonFeatures.Clients NAMESPACE Gem
    TARGETS
        Gem::AtomLyIntegration_CommonFeatures
        Gem::GradientSignal.Clients
)
ly_create_alias(NAME AtomLyIntegration_CommonFeatures.Servers NAMESPACE Gem
    TARGETS
        Gem::AtomLyIntegration_CommonFeatures
        Gem::GradientSignal.Servers
)
ly_create_alias(NAME AtomLyIntegration_CommonFeatures.Unified NAMESPACE Gem
    TARGETS
        Gem::AtomLyIntegration_CommonFeatures
        Gem::GradientSignal.Unified
)

# Add aliases that references the real name of the gem "CommonFeaturesAtom"
# that reference the names Gem variants of "AtomLyIntegration_CommonFeatures"
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem
    TARGETS
        Gem::AtomLyIntegration_CommonFeatures.Clients
)
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem
    TARGETS
        Gem::AtomLyIntegration_CommonFeatures.Servers
)
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem
    TARGETS
        Gem::AtomLyIntegration_CommonFeatures.Unified
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME AtomLyIntegration_CommonFeatures.Editor.Static STATIC
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
                Gem::AtomLyIntegration_CommonFeatures.Static
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
        NAME AtomLyIntegration_CommonFeatures.Editor GEM_MODULE
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
                Gem::AtomLyIntegration_CommonFeatures.Editor.Static
        RUNTIME_DEPENDENCIES
            Legacy::EditorCommon
    )

    # The AtomLyIntegration_CommonFeatures.Editor module is used for Builders and Tools
    ly_create_alias(NAME AtomLyIntegration_CommonFeatures.Builders NAMESPACE Gem
        TARGETS
            Gem::AtomLyIntegration_CommonFeatures.Editor
            Gem::Atom_Feature_Common.Builders
            Gem::Atom_RPI.Builders
            Gem::GradientSignal.Builders
    )
    ly_create_alias(NAME AtomLyIntegration_CommonFeatures.Tools NAMESPACE Gem
        TARGETS
            Gem::AtomLyIntegration_CommonFeatures.Editor
            Gem::GradientSignal.Tools
    )

    # Add aliases that references the real name of the gem "CommonFeaturesAtom"
    # Tools and Builders variants
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem
        TARGETS
            Gem::AtomLyIntegration_CommonFeatures.Builders
    )
    ly_create_alias(NAME ${gem_name}.Tools NAMESPACE Gem
        TARGETS
            Gem::AtomLyIntegration_CommonFeatures.Tools
    )

    ################################################################################
    # Tests
    ################################################################################
    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        ly_add_target(
            NAME AtomLyIntegration_CommonFeatures.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                    Gem::AtomLyIntegration_CommonFeatures.Static
                    Gem::AtomLyIntegration_CommonFeatures.Editor.Static
            RUNTIME_DEPENDENCIES
                Gem::LmbrCentral.Editor
        )
        ly_add_googletest(
            NAME Gem::AtomLyIntegration_CommonFeatures.Editor.Tests
            LABELS REQUIRES_tiaf
        )
    endif()
endif()

# AtomLyIntegration_CommonFeatures gem targets are required as part of the Editor and AssetProcessor
# due to the AZ::Render::EditorDirectionalLightComponent, AZ::Render::EditorMeshComponent,
# AZ::Render::EditorGridComponent, AZ::Render::EditorHDRiSkyboxComponent,
# AZ::Render::EditorImageBasedLightComponent being saved as part of the DefaultLevel.prefab
