#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

# Enable the enabled_gems for the Project:
ly_enable_gems(PROJECT_NAME AutomatedTesting GEM_FILE enabled_gems.cmake)

# Add project to the list server projects to create the AutomatedTesting.ServerLauncher
if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
    set_property(GLOBAL APPEND PROPERTY LY_LAUNCHER_SERVER_PROJECTS AutomatedTesting)
endif()

