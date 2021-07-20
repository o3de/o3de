#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME AutomatedTesting ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        automatedtesting_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::Atom_AtomBridge.Static
)

# if enabled, AutomatedTesting is used by all kinds of applications
ly_create_alias(NAME AutomatedTesting.Builders NAMESPACE Gem TARGETS Gem::AutomatedTesting)
ly_create_alias(NAME AutomatedTesting.Tools    NAMESPACE Gem TARGETS Gem::AutomatedTesting)
ly_create_alias(NAME AutomatedTesting.Clients  NAMESPACE Gem TARGETS Gem::AutomatedTesting)
ly_create_alias(NAME AutomatedTesting.Servers  NAMESPACE Gem TARGETS Gem::AutomatedTesting)

################################################################################
# Gem dependencies
################################################################################

# The GameLauncher uses "Clients" gem variants:
ly_enable_gems(PROJECT_NAME AutomatedTesting GEM_FILE enabled_gems.cmake
    TARGETS AutomatedTesting.GameLauncher
    VARIANTS Clients)

# If we build a server, then apply the gems to the server
if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
    # if we're making a server, then add the "Server" gem variants to it:
    ly_enable_gems(PROJECT_NAME AutomatedTesting GEM_FILE enabled_gems.cmake
        TARGETS AutomatedTesting.ServerLauncher
        VARIANTS Servers)
    
    set_property(GLOBAL APPEND PROPERTY LY_LAUNCHER_SERVER_PROJECTS AutomatedTesting)
endif()

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    # The Editor uses "Tools" gem variants:
    ly_enable_gems(
        PROJECT_NAME AutomatedTesting GEM_FILE enabled_gems.cmake
        TARGETS Editor
        VARIANTS Tools)

    # The pipeline tools use "Builders" gem variants:
    ly_enable_gems(
        PROJECT_NAME AutomatedTesting GEM_FILE enabled_gems.cmake
        TARGETS AssetBuilder AssetProcessor AssetProcessorBatch
        VARIANTS Builders)
endif()
