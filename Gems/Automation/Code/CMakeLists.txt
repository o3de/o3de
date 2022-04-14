#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME Automation.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        automation_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

ly_add_target(
    NAME Automation ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        automation_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Automation.Static
)

ly_create_alias(NAME Automation.Servers NAMESPACE Gem TARGETS Gem::Automation)
ly_create_alias(NAME Automation.Builders NAMESPACE Gem TARGETS Gem::Automation)
ly_create_alias(NAME Automation.Clients NAMESPACE Gem TARGETS Gem::Automation)
ly_create_alias(NAME Automation.Tools NAMESPACE Gem TARGETS Gem::Automation)

