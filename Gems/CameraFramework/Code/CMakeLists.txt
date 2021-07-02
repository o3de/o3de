#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME CameraFramework.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        cameraframework_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
)

ly_add_target(
    NAME CameraFramework ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        cameraframework_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::CameraFramework.Static
)

# Every kind of application uses the above target module.
ly_create_alias(NAME CameraFramework.Clients  NAMESPACE Gem TARGETS Gem::CameraFramework)
ly_create_alias(NAME CameraFramework.Servers  NAMESPACE Gem TARGETS Gem::CameraFramework)
ly_create_alias(NAME CameraFramework.Tools    NAMESPACE Gem TARGETS Gem::CameraFramework)
ly_create_alias(NAME CameraFramework.Builders NAMESPACE Gem TARGETS Gem::CameraFramework)
