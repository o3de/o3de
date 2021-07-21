#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME VideoPlaybackFramework.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        videoplaybackframework_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            .
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
        PRIVATE
            AZ::AzFramework
)

ly_add_target(
    NAME VideoPlaybackFramework ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        videoplaybackframework_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::VideoPlaybackFramework.Static
)

# the video playback framework makes sense in everything but servers:
ly_create_alias(NAME VideoPlaybackFramework.Clients  NAMESPACE Gem TARGETS Gem::VideoPlaybackFramework)
ly_create_alias(NAME VideoPlaybackFramework.Tools    NAMESPACE Gem TARGETS Gem::VideoPlaybackFramework)
ly_create_alias(NAME VideoPlaybackFramework.Builders NAMESPACE Gem TARGETS Gem::VideoPlaybackFramework)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME VideoPlaybackFramework.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            videoplaybackframework_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::VideoPlaybackFramework.Static
    )
    ly_add_googletest(
        NAME Gem::VideoPlaybackFramework.Tests
    )
endif()
