#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME VirtualGamepad.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        virtualgamepad_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Legacy::CryCommon
            Gem::LyShine
)

ly_add_target(
    NAME VirtualGamepad ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        virtualgamepad_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::VirtualGamepad.Static
)

# the virtual gamepad is needed everywhere except servers:
ly_create_alias(NAME VirtualGamepad.Clients  NAMESPACE Gem TARGETS Gem::VirtualGamepad)
ly_create_alias(NAME VirtualGamepad.Tools    NAMESPACE Gem TARGETS Gem::VirtualGamepad)
ly_create_alias(NAME VirtualGamepad.Builders NAMESPACE Gem TARGETS Gem::VirtualGamepad)
