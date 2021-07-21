#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME SceneLoggingExample.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        sceneloggingexample_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::SceneCore
            Legacy::CryCommon
)

ly_add_target(
    NAME SceneLoggingExample ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        sceneloggingexample_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::SceneLoggingExample.Static
)

# the SceneLoggingExample module above is only used in Builders and Tools by default.
ly_create_alias(NAME SceneLoggingExample.Builders NAMESPACE Gem TARGETS Gem::SceneLoggingExample)
ly_create_alias(NAME SceneLoggingExample.Tools    NAMESPACE Gem TARGETS Gem::SceneLoggingExample)
