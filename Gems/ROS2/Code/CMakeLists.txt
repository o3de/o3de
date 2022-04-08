# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT

# If no ROS2 is found, no targets are valid for this Gem
find_package(ROS2 MODULE)
if (NOT ROS2_FOUND)
    return()
endif()

# Add the ROS2.Static target
# Note: We include the common files and the platform specific files which are set in ros2_common_files.cmake
# and in ${pal_dir}/ros2_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME ROS2.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        ros2_files.cmake
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

target_depends_on_ros2_packages(ROS2.Static rclcpp builtin_interfaces std_msgs)

# Here add ROS2 target, it depends on the ROS2.Static
ly_add_target(
    NAME ROS2 ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        ros2_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::ROS2.Static
)

# By default, we will specify that the above target ROS2 would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ROS2.Clients NAMESPACE Gem TARGETS Gem::ROS2)
ly_create_alias(NAME ROS2.Servers NAMESPACE Gem TARGETS Gem::ROS2)

# If we are on a host platform, we want to add the host tools targets like the ROS2.Editor target which
# will also depend on ROS2.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ROS2.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            ros2_editor_files.cmake
        PLATFORM_INCLUDE_FILES
            ${CMAKE_CURRENT_LIST_DIR}/Platform/Common/${PAL_TRAIT_COMPILER_ID}/ros2_editor_static_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::ROS2.Static
    )

    ly_add_target(
        NAME ROS2.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            ros2_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::ROS2.Editor.Static
    )

    # By default, we will specify that the above target ROS2 would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME ROS2.Tools    NAMESPACE Gem TARGETS Gem::ROS2.Editor)
    ly_create_alias(NAME ROS2.Builders NAMESPACE Gem TARGETS Gem::ROS2.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ROS2.Static
    if(PAL_TRAIT_ROS2_TEST_SUPPORTED)
        # We support ROS2.Tests on this platform, add ROS2.Tests target which depends on ROS2.Static
        ly_add_target(
            NAME ROS2.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                ros2_files.cmake
                ros2_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::ROS2.Static
        )

        # Add ROS2.Tests to googletest
        ly_add_googletest(
            NAME Gem::ROS2.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_ROS2_EDITOR_TEST_SUPPORTED)
            # We support ROS2.Editor.Tests on this platform, add ROS2.Editor.Tests target which depends on ROS2.Editor
            ly_add_target(
                NAME ROS2.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    ros2_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::ROS2.Editor
            )

            # Add ROS2.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::ROS2.Editor.Tests
            )
        endif()
    endif()
endif()
