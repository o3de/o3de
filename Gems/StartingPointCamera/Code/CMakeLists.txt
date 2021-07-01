#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME StartingPointCamera.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        startingpointcamera_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::CameraFramework.Static
            Gem::LmbrCentral
            Legacy::CryCommon
)

ly_add_target(
    NAME StartingPointCamera ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        startingpointcamera_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::StartingPointCamera.Static
            Gem::CameraFramework.Static
            Gem::LmbrCentral.Static
            Legacy::CryCommon
    RUNTIME_DEPENDENCIES
        Gem::CameraFramework
)

# the above module is for use in all kinds of applications
ly_create_alias(NAME StartingPointCamera.Servers  NAMESPACE Gem TARGETS Gem::StartingPointCamera)
ly_create_alias(NAME StartingPointCamera.Clients  NAMESPACE Gem TARGETS Gem::StartingPointCamera)
ly_create_alias(NAME StartingPointCamera.Builders NAMESPACE Gem TARGETS Gem::StartingPointCamera)
ly_create_alias(NAME StartingPointCamera.Tools    NAMESPACE Gem TARGETS Gem::StartingPointCamera)
