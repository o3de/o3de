#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Only declare this gem's targets when the gem is enabled for at least one configured
# project. The AssImp subdirectory is added for every project (engine.json
# "external_subdirectories"), so without this check the assimp 3rdParty package would be
# fetched and built even for projects that disabled the gem.
# Engine builds without any project (for example the SDK/installer build) always build the
# gem so that it is available to projects created from that engine.
set(O3DE_ASSIMP_GEM_BUILD "AUTO" CACHE STRING
    "Build the AssImp gem: AUTO (when enabled by a configured project, or when no project is configured), ON, or OFF")
set_property(CACHE O3DE_ASSIMP_GEM_BUILD PROPERTY STRINGS AUTO ON OFF)

set(assimp_gem_enabled FALSE)
get_property(o3de_project_names GLOBAL PROPERTY O3DE_PROJECTS_NAME)
if (O3DE_ASSIMP_GEM_BUILD STREQUAL "ON" OR (O3DE_ASSIMP_GEM_BUILD STREQUAL "AUTO" AND NOT o3de_project_names))
    set(assimp_gem_enabled TRUE)
elseif (O3DE_ASSIMP_GEM_BUILD STREQUAL "OFF")
    set(o3de_project_names "")
endif()
foreach(project_name ${o3de_project_names})
    # Note: the LY_DELAYED_ENABLE_GEMS_<name> property names intentionally contain
    # quote characters, matching how ly_enable_gems stores them in cmake/Gems.cmake.
    get_property(project_enabled_gems GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS_"${project_name}")
    if (gem_name IN_LIST project_enabled_gems)
        set(assimp_gem_enabled TRUE)
        break()
    endif()
endforeach()
# Gems can also be enabled engine-wide (the "__NOPROJECT__" scope used by mandatory gems).
if (NOT assimp_gem_enabled AND NOT O3DE_ASSIMP_GEM_BUILD STREQUAL "OFF")
    get_property(engine_enabled_gems GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS_"__NOPROJECT__")
    if (gem_name IN_LIST engine_enabled_gems)
        set(assimp_gem_enabled TRUE)
    endif()
endif()

if (NOT assimp_gem_enabled)
    message(STATUS "AssImp gem is not enabled by any configured project; skipping its targets and the assimp library")
    return()
endif()

# This is a Tool-only gem: it declares no base runtime module. Applications resolve the
# gem's module by variant name (Gem::AssImp.Builders / Gem::AssImp.Tools below), so there
# is no stub module to carry.
if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            assimp_editor_static_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                .
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::assimp
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::SceneCore
                AZ::SceneData
                AZ::SceneBuilder.Static
                AZ::SDKWrapper
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            assimp_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                .
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::${gem_name}.Editor.Static
        RUNTIME_DEPENDENCIES
            AZ::SceneCore
            AZ::SceneData
            AZ::SceneBuilder
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/AssImpModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    # The ${gem_name}.Editor module is used in Builders and Tools and is required
    # for the Editor and AssetProcessor when the AssImp importer is enabled.
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
endif()
