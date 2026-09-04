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
            Source/Maestro
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
            Source/Maestro
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

ly_create_alias(NAME ${gem_name}.Static NAMESPACE Gem TARGETS Gem::${gem_name}.Private.Object)
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Clients.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Servers.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Unified.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)

o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Clients Servers Unified)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            maestro_files.cmake
            maestro_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                MAESTRO_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Widgets
                Legacy::CryCommon
                Legacy::Editor.Headers
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                Gem::${gem_name}.Static
            PUBLIC
                Gem::Atom_Bootstrap.Headers
                Gem::Atom_RHI.Public
                Gem::Atom_RHI.Reflect
                Gem::Atom_RPI.Public

        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )
    # the .Editor variant is used in dev tools:
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

endif()

# ${gem_name} is still used by the CrySystem Level System, CSystem::SystemInit and TrackView

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.API.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            maestro_api_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
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
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::CryCommon
                Gem::${gem_name}.Static
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
        LABELS REQUIRES_tiaf
    )
endif()
