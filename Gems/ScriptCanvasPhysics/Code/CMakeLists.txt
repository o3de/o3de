#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ScriptCanvasPhysics.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvas_physics_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::ScriptCanvas
)

ly_add_target(
    NAME ScriptCanvasPhysics ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvas_physics_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::ScriptCanvasPhysics.Static
            Gem::ScriptCanvas.API
)

# By default, the above module is used by all application types, however, the module depends at runtime to ScriptCanvas
# and the dependency needs to be different per application type
ly_create_alias(NAME ScriptCanvasPhysics.Clients  NAMESPACE Gem TARGETS Gem::ScriptCanvasPhysics Gem::ScriptCanvas)
ly_create_alias(NAME ScriptCanvasPhysics.Servers  NAMESPACE Gem TARGETS Gem::ScriptCanvasPhysics Gem::ScriptCanvas)
ly_create_alias(NAME ScriptCanvasPhysics.Tools    NAMESPACE Gem TARGETS Gem::ScriptCanvasPhysics Gem::ScriptCanvas.Editor)
ly_create_alias(NAME ScriptCanvasPhysics.Builders NAMESPACE Gem TARGETS Gem::ScriptCanvasPhysics Gem::ScriptCanvas.Editor)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ScriptCanvasPhysics.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            scriptcanvas_physics_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzFramework
                AZ::AzTest
                Legacy::CryCommon
                Gem::ScriptCanvasPhysics.Static
                Gem::ScriptCanvas
    )
    ly_add_googletest(
        NAME Gem::ScriptCanvasPhysics.Tests
    )
endif()
