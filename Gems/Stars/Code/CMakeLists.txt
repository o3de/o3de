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
    NAME ${gem_name}.API HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        stars_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            include
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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
            Gem::${gem_name}.Static
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/StarsModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor.Static STATIC
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
                Gem::${gem_name}.Static
                Gem::Atom_Feature_Common.Public
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
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
                Gem::${gem_name}.Editor.Static
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/EditorStarsModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    # A Builders alias must exist for Gem Assets to be seen by the Asset Processor.
    # A Builders alias target must exist for the Prefab .spawnable builder to be able to
    # load the .dll so the builder can serialize StarComponents to .spawnable files 
    # which are needed in the game launcher.
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
endif()
