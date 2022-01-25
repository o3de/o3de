#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    # Declare ATOMTRESSFX_EDITOR only when tools pipeline is built
    # This is in order to allow AzToolsFramework to properly link across
    # platforms that support editor mode, while in the rest, not include
    # this module and avoide compiling the editor components.
    set_source_files_properties(
        Code/HairModule.cpp
        Code/Components/EditorHairComponent.h
        Code/Components/EditorHairComponent.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                ATOMTRESSFX_EDITOR
    )

    ly_add_target(
        NAME AtomTressFX.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            Hair_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code
                External
                External/Code/src
            PUBLIC
                Code/Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzToolsFramework
                Gem::LmbrCentral
                Gem::Atom_RHI.Public
                Gem::Atom_RPI.Public
                Gem::Atom_Feature_Common.Static
                Gem::AtomLyIntegration_CommonFeatures.Static
                Gem::EMotionFXStaticLib
    )

    ly_add_target(
        NAME AtomTressFX ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            Hair_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code/AssetPipeline
                Code
                External
                External/Code/src
            PUBLIC
                Code/Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzToolsFramework
                Gem::Atom_Utils.Static
                Gem::EMotionFXStaticLib
                Gem::AtomTressFX.Static
    )

else()

    ly_add_target(
        NAME AtomTressFX.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            Hair_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code
                External
                External/Code/src
            PUBLIC
                Code/Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Gem::LmbrCentral
                Gem::Atom_RHI.Public
                Gem::Atom_RPI.Public
                Gem::Atom_Feature_Common.Static
                Gem::AtomLyIntegration_CommonFeatures.Static
                Gem::EMotionFXStaticLib
    )

    ly_add_target(
        NAME AtomTressFX ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            Hair_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code/AssetPipeline
                Code
                External
                External/Code/src
            PUBLIC
                Code/Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Gem::Atom_Utils.Static
                Gem::EMotionFXStaticLib
                Gem::AtomTressFX.Static
    )
endif()

# Clients and servers use the AtomTressFX module
ly_create_alias(NAME AtomTressFX.Clients NAMESPACE Gem TARGETS Gem::AtomTressFX)
ly_create_alias(NAME AtomTressFX.Servers NAMESPACE Gem TARGETS Gem::AtomTressFX)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME AtomTressFX.Builders.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            Hair_builders_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code
                External
                External/Code/src
            PUBLIC
                Code/Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AssetBuilderSDK
                Gem::AtomTressFX.Static
    )

    ly_add_target(
        NAME AtomTressFX.Builders GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            Hair_builders_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code
                External
                External/Code/src
            PUBLIC
                Code/Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::AtomTressFX.Builders.Static
    )
    
    # builders and tools use the AtomTressFX.Builders and AtomTressFX modules
    ly_create_alias(NAME AtomTressFX.Tools   NAMESPACE Gem TARGETS Gem::AtomTressFX Gem::AtomTressFX.Builders)

endif()
