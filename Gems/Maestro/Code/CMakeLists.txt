#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME Maestro.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        maestro_static_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
)

ly_add_target(
    NAME Maestro ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        maestro_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::Maestro.Static
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# if enabled, "Maestro" module is used for Clients and Servers:
ly_create_alias(NAME Maestro.Clients  NAMESPACE Gem TARGETS Gem::Maestro)
ly_create_alias(NAME Maestro.Servers  NAMESPACE Gem TARGETS Gem::Maestro)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Maestro.Editor GEM_MODULE

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
                Gem::Maestro.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )
    # the .Editor variant is used in dev tools:
    ly_create_alias(NAME Maestro.Tools    NAMESPACE Gem TARGETS Gem::Maestro.Editor)
    ly_create_alias(NAME Maestro.Builders NAMESPACE Gem TARGETS Gem::Maestro.Editor)

    # Maestro is still used by the CrySystem Level System and SystemInit and TrackView
    # It is required by the GameLauncher, ServerLauncher and Editor applications
    ly_enable_gems(GEMS Maestro VARIANTS Tools TARGETS Editor)

endif()

# Loop over each Project name to allow the ${ProjectName}.GameLauncher and ${ProjectName}.ServerLauncher
# target to add the gem the Clients and Servers variant
get_property(LY_PROJECTS_TARGET_NAME GLOBAL PROPERTY LY_PROJECTS_TARGET_NAME)
foreach(project_name IN LISTS LY_PROJECTS_TARGET_NAME)
    # Add gem as a dependency of the Clients Launcher
    ly_enable_gems(PROJECT_NAME ${project_name} GEMS Maestro VARIANTS Clients TARGETS ${project_name}.GameLauncher)
    # Add gem as a dependency of the Servers Launcher
    if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
        ly_enable_gems(PROJECT_NAME ${project_name} GEMS Maestro VARIANTS Servers TARGETS ${project_name}.ServerLauncher)
    endif()
endforeach()


################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME Maestro.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            maestro_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::CryCommon
                Gem::Maestro.Static
    )
    ly_add_googletest(
        NAME Gem::Maestro.Tests
    )
endif()
