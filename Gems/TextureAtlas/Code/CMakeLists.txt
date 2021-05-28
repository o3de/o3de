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
    NAME TextureAtlas.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        textureatlas_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            AZ::AzFramework
        PUBLIC
            Gem::Atom_RPI.Public
            AZ::AtomCore
)

ly_add_target(
    NAME TextureAtlas ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        textureatlas_module_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::TextureAtlas.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME TextureAtlas.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            textureatlas_module_files.cmake
            textureatlas_builder_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                TEXTUREATLAS_EDITOR
        INCLUDE_DIRECTORIES
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommon
                AZ::AzCore
                AZ::AzFramework
                AZ::AssetBuilderSDK
                Gem::TextureAtlas.Static
                Gem::ImageProcessingAtom.Headers
    )
    ly_create_alias(NAME TextureAtlas.Builders NAMESPACE Gem TARGETS Gem::TextureAtlas.Editor)
    ly_create_alias(NAME TextureAtlas.Tools    NAMESPACE Gem TARGETS Gem::TextureAtlas.Editor)
endif()

ly_create_alias(NAME TextureAtlas.Servers  NAMESPACE Gem TARGETS Gem::TextureAtlas)
ly_create_alias(NAME TextureAtlas.Clients  NAMESPACE Gem TARGETS Gem::TextureAtlas)

