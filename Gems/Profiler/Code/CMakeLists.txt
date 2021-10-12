#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Add the Profiler.Static target
ly_add_target(
    NAME Profiler.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        profiler_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::ImGui.Static
)

# Here add Profiler target, it depends on the Profiler.Static
ly_add_target(
    NAME Profiler ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        profiler_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Profiler.Static
    RUNTIME_DEPENDENCIES
        Gem::ImGui
)

# By default, we will specify that the above target Profiler would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME Profiler.Clients NAMESPACE Gem TARGETS Gem::Profiler)
ly_create_alias(NAME Profiler.Servers NAMESPACE Gem TARGETS Gem::Profiler)
