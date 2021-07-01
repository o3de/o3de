#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME Atom_Bootstrap.Headers HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        bootstrap_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
)

ly_add_target(
    NAME Atom_Bootstrap ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        bootstrap_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AtomCore
            AZ::AzCore
            AZ::AzFramework
            Legacy::CryCommon
            Gem::Atom_RPI.Public
)

# Atom_Bootstrap is only used in Launchers
ly_create_alias(NAME Atom_Bootstrap.Clients NAMESPACE Gem TARGETS Gem::Atom_Bootstrap)
ly_create_alias(NAME Atom_Bootstrap.Servers NAMESPACE Gem TARGETS Gem::Atom_Bootstrap)

# The Atom_Bootstrap gem is responsible for making the NativeWindow handle in the launcher applications
# Loop over each Project name to allow the ${ProjectName}.GameLauncher and ${ProjectName}.ServerLauncher
# target to add the gem the Clients and Servers variant
get_property(LY_PROJECTS_TARGET_NAME GLOBAL PROPERTY LY_PROJECTS_TARGET_NAME)
foreach(project_name IN LISTS LY_PROJECTS_TARGET_NAME)
    # Add gem as a dependency of the Clients Launcher
    ly_enable_gems(PROJECT_NAME ${project_name} GEMS Atom_Bootstrap VARIANTS Clients TARGETS ${project_name}.GameLauncher)
    # Add gem as a dependency of the Servers Launcher
    if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
        ly_enable_gems(PROJECT_NAME ${project_name} GEMS Atom_Bootstrap VARIANTS Servers TARGETS ${project_name}.ServerLauncher)
    endif()
endforeach()
