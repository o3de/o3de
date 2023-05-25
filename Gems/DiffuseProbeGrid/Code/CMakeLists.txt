#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

ly_add_target(
    NAME DiffuseProbeGrid.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        diffuseprobegrid_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
            ${pal_source_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_RPI.Public
            Gem::Atom_Utils.Static
            Gem::Atom_Feature_Common
            Gem::AtomLyIntegration_CommonFeatures.Static
            Gem::LmbrCentral
)

ly_add_target(
    NAME DiffuseProbeGrid ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        diffuseprobegrid_shared_files.cmake
       ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
            ${pal_source_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::DiffuseProbeGrid.Static
            Gem::LmbrCentral.API
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# the above module is for use in all client/server types
ly_create_alias(NAME DiffuseProbeGrid.Servers  NAMESPACE Gem TARGETS Gem::DiffuseProbeGrid)
ly_create_alias(NAME DiffuseProbeGrid.Clients  NAMESPACE Gem TARGETS Gem::DiffuseProbeGrid)
ly_create_alias(NAME DiffuseProbeGrid.Unified  NAMESPACE Gem TARGETS Gem::DiffuseProbeGrid)

# If we are on a host platform add DiffuseProbeGrid.Editor which will also depend on DiffuseProbeGrid.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME DiffuseProbeGrid.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            diffuseprobegrid_editor_shared_files.cmake
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                DIFFUSEPROBEGRID_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::DiffuseProbeGrid.Static
    )

    ly_create_alias(NAME DiffuseProbeGrid.Builders NAMESPACE Gem TARGETS Gem::DiffuseProbeGrid.Editor)
    ly_create_alias(NAME DiffuseProbeGrid.Tools    NAMESPACE Gem TARGETS Gem::DiffuseProbeGrid.Editor)
endif()
