#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME StartingPointMovement ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        startingpointmovement_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
)

# the above module is for use in all application types (there is no tool specialization)
ly_create_alias(NAME StartingPointMovement.Servers  NAMESPACE Gem TARGETS Gem::StartingPointMovement)
ly_create_alias(NAME StartingPointMovement.Clients  NAMESPACE Gem TARGETS Gem::StartingPointMovement)
ly_create_alias(NAME StartingPointMovement.Builders NAMESPACE Gem TARGETS Gem::StartingPointMovement)
ly_create_alias(NAME StartingPointMovement.Tools    NAMESPACE Gem TARGETS Gem::StartingPointMovement)