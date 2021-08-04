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

set(GemUUID d4a3e3d6b9b14e12982c774f35f48a7c)

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
            ./Code/Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzToolsFramework
            Gem::Atom_Utils.Static
            Gem::EMotionFXStaticLib
            Gem::AtomTressFX.Static
)

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

    ly_create_alias(NAME AtomTressFX.Tools   NAMESPACE Gem TARGETS Gem::AtomTressFX Gem::AtomTressFX.Builders)

endif()
