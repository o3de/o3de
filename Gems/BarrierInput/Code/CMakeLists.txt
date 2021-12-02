#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME BarrierInput.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        barrierinput_files.cmake
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
    RUNTIME_DEPENDENCIES
        Gem::Atom_RPI.Private
)

ly_add_target(
    NAME BarrierInput ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        barrierinput_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::BarrierInput.Static
)

# Barrier Input is only needed for the client:
ly_create_alias(NAME BarrierInput.Clients  NAMESPACE Gem TARGETS Gem::BarrierInput)
