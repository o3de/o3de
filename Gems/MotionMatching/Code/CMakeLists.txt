#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Add the MotionMatching.Static target
ly_add_target(
    NAME MotionMatching.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        motionmatching_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::EMotionFXStaticLib
            Gem::ImguiAtom.Static
)

# Here add MotionMatching target, it depends on the MotionMatching.Static
ly_add_target(
    NAME MotionMatching ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        motionmatching_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::MotionMatching.Static
            Gem::ImGui.Static
            Gem::ImGui.ImGuiLYUtils
)

# By default, we will specify that the above target MotionMatching would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME MotionMatching.Clients NAMESPACE Gem TARGETS Gem::MotionMatching)
ly_create_alias(NAME MotionMatching.Servers NAMESPACE Gem TARGETS Gem::MotionMatching)

# If we are on a host platform, we want to add the host tools targets like the MotionMatching.Editor target which
# will also depend on MotionMatching.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME MotionMatching.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            motionmatching_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::MotionMatching.Static
    )

    ly_add_target(
        NAME MotionMatching.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        OUTPUT_NAME Gem.MotionMatching.Editor
        FILES_CMAKE
            motionmatching_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::MotionMatching.Editor.Static
    )

    # By default, we will specify that the above target MotionMatching would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME MotionMatching.Tools    NAMESPACE Gem TARGETS Gem::MotionMatching.Editor)
    ly_create_alias(NAME MotionMatching.Builders NAMESPACE Gem TARGETS Gem::MotionMatching.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME MotionMatching.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            motionmatching_files.cmake
            motionmatching_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                Gem::EMotionFX.Tests.Static
                Gem::MotionMatching.Static
    )

    # Add MotionMatching.Tests to googletest
    ly_add_googletest(
        NAME Gem::MotionMatching.Tests
    )
endif()

# If we are a host platform we want to add tools test like editor tests here
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME MotionMatching.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            motionmatching_editor_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::MotionMatching.Editor
    )

    # Add MotionMatching.Editor.Tests to googletest
    ly_add_googletest(
        NAME Gem::MotionMatching.Editor.Tests
    )
endif()
