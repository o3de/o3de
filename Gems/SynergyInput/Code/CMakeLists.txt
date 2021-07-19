#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME SynergyInput.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        synergyinput_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            AZ::AtomCore
            Gem::Atom_RPI.Public
)

ly_add_target(
    NAME SynergyInput ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        synergyinput_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::SynergyInput.Static
)

# Synergy Input is only needed for the client:
ly_create_alias(NAME SynergyInput.Clients  NAMESPACE Gem TARGETS Gem::SynergyInput)
