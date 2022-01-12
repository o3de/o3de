#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME EMotionFX_Atom.Static STATIC
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
            Gem::Atom_Feature_Common.Static
            Gem::Atom_RPI.Public
            Gem::Atom_RHI.Reflect
            Gem::AtomLyIntegration_CommonFeatures.Public
)

ly_add_target(
    NAME EMotionFX_Atom ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        emotionfxatom_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::EMotionFX_Atom.Static
    RUNTIME_DEPENDENCIES
        Gem::EMotionFX
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME EMotionFX_Atom.Editor GEM_MODULE

        NAMESPACE Gem
        AUTORCC
        FILES_CMAKE
            emotionfx_atom_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tools
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Gem::EMotionFX_Atom.Static
                Gem::EMotionFX.Editor.Static
                Gem::AtomToolsFramework.Static
                Gem::AtomToolsFramework.Editor
                Gem::Atom_Component_DebugCamera.Static
                Gem::Atom_Feature_Common.Static
                Gem::AtomLyIntegration_CommonFeatures.Static
        RUNTIME_DEPENDENCIES
            Gem::EMotionFX.Editor
        COMPILE_DEFINITIONS
            PUBLIC
            EMOTIONFXATOM_EDITOR
    )
endif()
