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
    NAME ScriptCanvasPhysics.Static STATIC
    NAMESPACE Gem
    OUTPUT_NAME Gem.ScriptCanvasPhysics.Static
    FILES_CMAKE
        scriptcanvas_physics_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ScriptCanvas
        PRIVATE
            Legacy::CryCommon
)

ly_add_target(
    NAME ScriptCanvasPhysics ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.ScriptCanvasPhysics.1c27519a4dda4ffaaeebf91514e5b1e8.v0.1.0
    FILES_CMAKE
        scriptcanvas_physics_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::ScriptCanvasPhysics.Static
)

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
    )
    ly_add_googletest(
        NAME Gem::ScriptCanvasPhysics.Tests
    )
endif()
