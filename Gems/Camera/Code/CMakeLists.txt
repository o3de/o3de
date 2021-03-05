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
        PRIVATE
            Legacy::CryCommon
)

ly_add_target(
    NAME Camera ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Camera.f910686b6725452fbfc4671f95f733c6.v0.1.0
    FILES_CMAKE
        camera_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::Camera.Static
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Camera.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Camera.Editor.f910686b6725452fbfc4671f95f733c6.v0.1.0
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
                Legacy::CryCommon
                Legacy::EditorLib
                AZ::AzToolsFramework
                Gem::Camera.Static
    )

endif()
