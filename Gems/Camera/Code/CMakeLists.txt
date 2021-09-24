#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME Camera.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        camera_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::Atom_RPI.Public
            AZ::AtomCore
            AZ::AzCore
            AZ::AzFramework
)

ly_add_target(
    NAME Camera ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        camera_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Camera.Static
)

# clients and servers use the above module:
ly_create_alias(NAME Camera.Clients NAMESPACE Gem TARGETS Gem::Camera)
ly_create_alias(NAME Camera.Servers NAMESPACE Gem TARGETS Gem::Camera)
   
if (PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Camera.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            camera_editor_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                CAMERA_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzToolsFramework
                Gem::Camera.Static
                Gem::AtomToolsFramework.Static
        RUNTIME_DEPENDENCIES
            Legacy::EditorCommon
    )

    # tools and builders use the above module.
    ly_create_alias(NAME Camera.Tools NAMESPACE    Gem TARGETS Gem::Camera.Editor)
    ly_create_alias(NAME Camera.Builders NAMESPACE Gem TARGETS Gem::Camera.Editor)
endif()

# The DefaultPrefab contains an EditorCameraComponent which makes this gem required
ly_enable_gems(GEMS Camera)
