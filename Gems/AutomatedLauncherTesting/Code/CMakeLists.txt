#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME AutomatedLauncherTesting.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        automatedlaunchertesting_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Legacy::CryCommon
            Gem::LmbrCentral
)

ly_add_target(
    NAME AutomatedLauncherTesting ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        automatedlaunchertesting_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AutomatedLauncherTesting.Static
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# servers and clients use the above module.
ly_create_alias(NAME AutomatedLauncherTesting.Servers NAMESPACE Gem TARGETS Gem::AutomatedLauncherTesting)
ly_create_alias(NAME AutomatedLauncherTesting.Clients NAMESPACE Gem TARGETS Gem::AutomatedLauncherTesting)
