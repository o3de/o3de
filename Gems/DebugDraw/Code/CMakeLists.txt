#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# The DebugDraw.API target declares the common interface that users of this gem should depend on in their targets to avoid linking.
ly_add_target(
    NAME DebugDraw.API INTERFACE
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
    NAME DebugDraw.Static STATIC
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
            Legacy::CryCommon
)

ly_add_target(
    NAME DebugDraw ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        debugdraw_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::DebugDraw.Static
)

# servers do not need debug draw components, only clients
ly_create_alias(NAME DebugDraw.Clients NAMESPACE Gem TARGETS DebugDraw)
ly_create_alias(NAME DebugDraw.Unified NAMESPACE Gem TARGETS DebugDraw)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME DebugDraw.Editor GEM_MODULE

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
                Legacy::CryCommon
                AZ::AzToolsFramework
    )

    # builders and tools use DebugDraw.Editor
    ly_create_alias(NAME DebugDraw.Builders NAMESPACE Gem TARGETS DebugDraw.Editor)
    ly_create_alias(NAME DebugDraw.Tools    NAMESPACE Gem TARGETS DebugDraw.Editor)
  
endif()
