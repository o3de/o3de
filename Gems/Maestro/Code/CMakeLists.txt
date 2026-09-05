#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ${gem_name}.API STATIC
    NAMESPACE Gem
    FILES_CMAKE
        maestro_api_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
            Gem::Atom_Bootstrap.Headers
            Gem::Atom_RHI.Public
            Gem::Atom_RHI.Reflect
            Gem::Atom_RPI.Public
            Gem::AudioSystem.API
)

ly_add_target(
    NAME ${gem_name}.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        maestro_static_files.cmake
    TARGET_PROPERTIES
        O3DE_PRIVATE_TARGET TRUE
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::${gem_name}.API
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        maestro_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::${gem_name}.API
        PRIVATE
            Gem::${gem_name}.Private.Object
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/Maestro/MaestroModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# Keep the legacy Static target as a regular interface target. ly_create_alias creates an imported
# target, which cannot safely copy generated include directories that may not exist during configure.
ly_add_target(
    NAME ${gem_name}.Static INTERFACE
    NAMESPACE Gem
    BUILD_DEPENDENCIES
        INTERFACE
            Gem::${gem_name}.Private.Object
)
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Clients.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Servers.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Unified.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)

o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Clients Servers Unified)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            maestro_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                3rdParty::Qt::Core
                AZ::AzToolsFramework
                Gem::${gem_name}.API
    )

    ly_add_target(
        NAME ${gem_name}.Editor.Private.Object STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        FILES_CMAKE
            maestro_editor_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Widgets
                Legacy::Editor.Headers
                Legacy::EditorLib
                Gem::Atom_Feature_Common.Public
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                Gem::${gem_name}.API
                Gem::${gem_name}.Editor.API
                Gem::${gem_name}.Private.Object
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        AUTORCC
        FILES_CMAKE
            maestro_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::${gem_name}.Editor.API
            PRIVATE
                Gem::${gem_name}.Private.Object
                Gem::${gem_name}.Editor.Private.Object
                Legacy::Editor.Headers
                Legacy::EditorLib
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )

    ly_add_source_properties(
        SOURCES
            Source/Maestro/Editor/MaestroEditorModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version}
    )

    # the .Editor variant is used in dev tools:
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)
    ly_create_alias(NAME ${gem_name}.Builders.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)

    o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Tools Builders)

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.API.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            maestro_api_tests_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::${gem_name}.API
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.API.Tests
        LABELS REQUIRES_tiaf
    )

    ly_add_target(
        NAME ${gem_name}.Static.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            maestro_static_compatibility_tests_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::${gem_name}.Static
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.Static.Tests
        LABELS REQUIRES_tiaf
    )

    ly_add_target(
        NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            maestro_tests_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                MAESTRO_TESTS
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::${gem_name}.API
                Gem::${gem_name}.Private.Object
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
        LABELS REQUIRES_tiaf
    )

    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                maestro_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Include
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    3rdParty::Qt::Widgets
                    Legacy::CryCommon
                    Legacy::Editor.Headers
                    Legacy::EditorLib
                    Gem::${gem_name}.Editor.API
                    Gem::${gem_name}.Editor.Private.Object
        )
        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
            LABELS REQUIRES_tiaf
        )
    endif()
endif()
