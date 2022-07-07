#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# data portion
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
)

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
)

ly_create_alias(NAME Profiler.Servers NAMESPACE Gem TARGETS Gem::Profiler)
ly_create_alias(NAME Profiler.Builders NAMESPACE Gem TARGETS Gem::Profiler)

# visualization portion
ly_add_target(
    NAME ProfilerImGui ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        profiler_imgui_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Profiler.Static
            Gem::ImGui.imguilib
    RUNTIME_DEPENDENCIES
        Gem::ImGui.imguilib
)

ly_create_alias(NAME Profiler.Clients NAMESPACE Gem TARGETS Gem::ProfilerImGui)
ly_create_alias(NAME Profiler.Tools NAMESPACE Gem TARGETS Gem::ProfilerImGui)
