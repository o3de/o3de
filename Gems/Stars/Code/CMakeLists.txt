#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME Stars.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        stars_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_RHI.Public
            Gem::Atom_RPI.Public
            Gem::Atom_Utils.Static
)

ly_add_target(
    NAME Stars.API HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        stars_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            include
)

ly_add_target(
    NAME Stars ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        stars_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Stars.Static
)

ly_create_alias(NAME Stars.Clients NAMESPACE Gem TARGETS Gem::Stars)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Stars.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            stars_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::Stars.Static
                Gem::Atom_Feature_Common.Static
                Gem::Atom_Feature_Common.Public
    )

    ly_add_target(
        NAME Stars.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            stars_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::Stars.Editor.Static
    )

    ly_create_alias(NAME Stars.Tools    NAMESPACE Gem TARGETS Gem::Stars.Editor)
    # A Builders alias must exist for Gem Assets to be seen by the Asset Processor.
    # A Builders alias target must exist for the Prefab .spawnable builder to be able to
    # load the .dll so the builder can serialize StarComponents to .spawnable files 
    # which are needed in the game launcher.
    ly_create_alias(NAME Stars.Builders NAMESPACE Gem TARGETS Gem::Stars.Editor)
endif()
