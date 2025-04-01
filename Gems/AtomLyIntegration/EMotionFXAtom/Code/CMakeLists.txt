#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        emotionfx_atom_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AtomCore
            Gem::EMotionFXStaticLib
            Gem::Atom_Utils.Static
            Gem::Atom_Feature_Common
            Gem::Atom_Feature_Common.Public
            Gem::Atom_RPI.Public
            Gem::Atom_RHI.Reflect
            Gem::CommonFeaturesAtom.Public
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        emotionfxatom_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::${gem_name}.Static
    RUNTIME_DEPENDENCIES
        Gem::EMotionFX
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/ActorModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE

        NAMESPACE Gem
        AUTORCC
        FILES_CMAKE
            emotionfx_atom_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Editor
                Source
                Tools
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                Gem::${gem_name}.Static
                Gem::EMotionFX.Editor.Static
                Gem::AtomToolsFramework.Static
                Gem::AtomToolsFramework.Editor
                Gem::Atom_Component_DebugCamera.Static
                Gem::Atom_Feature_Common.Public
                Gem::CommonFeaturesAtom.Static
        RUNTIME_DEPENDENCIES
            Gem::EMotionFX.Editor
        COMPILE_DEFINITIONS
            PUBLIC
            EMOTIONFXATOM_EDITOR
    )
endif()
