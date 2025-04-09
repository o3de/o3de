#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# The ${gem_name}.API target declares the common interface that users of this gem should depend on in their targets to avoid linking.
ly_add_target(
    NAME ${gem_name}.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        debugdraw_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
           AZ::AzCore
)

ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        debugdraw_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AtomCore
            Gem::Atom_RPI.Public
            Gem::Atom_Bootstrap.Headers
            Gem::Atom_Feature_Common.Public
            Legacy::CryCommon
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        debugdraw_shared_files.cmake
        ../Assets/debugdraw_asset_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Static
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/DebugDrawModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# servers do not need debug draw components, only clients
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS ${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS ${gem_name})

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            debugdraw_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                DEBUGDRAW_GEM_EDITOR=1
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AtomCore
                Gem::Atom_RPI.Public
                Gem::Atom_Bootstrap.Headers
                Gem::Atom_Feature_Common.Public
                Legacy::CryCommon
                AZ::AzToolsFramework
    )

    # builders and tools use ${gem_name}.Editor
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS ${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS ${gem_name}.Editor)
  
endif()
