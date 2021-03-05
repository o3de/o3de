#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

ly_add_target(
    NAME EMotionFX_Atom.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        emotionfx_atom_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
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
            Gem::AtomLyIntegration_CommonFeatures.Public
)

ly_add_target(
    NAME EMotionFX_Atom ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.EMotionFX_Atom.4f8a4d073ba34b43be45705f18705f1e.v0.1.0
    FILES_CMAKE
        emotionfxatom_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::EMotionFX_Atom.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME EMotionFX_Atom.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.EMotionFX_Atom.Editor.4f8a4d073ba34b43be45705f18705f1e.v0.1.0
        FILES_CMAKE
            emotionfx_atom_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Gem::EMotionFX_Atom.Static
    )
endif()
