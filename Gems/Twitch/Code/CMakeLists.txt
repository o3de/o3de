#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
set(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

ly_add_target(
    NAME Twitch.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        twitch_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Gem::HttpRequestor
            3rdParty::AWSNativeSDK::Core
    RUNTIME_DEPENDENCIES
        Gem::HttpRequestor
)

ly_add_target(
    NAME Twitch ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        twitch_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Twitch.Static
    RUNTIME_DEPENDENCIES
        Gem::HttpRequestor
)

# the above module is for use in all application types except builders
ly_create_alias(NAME Twitch.Servers  NAMESPACE Gem TARGETS Gem::Twitch)
ly_create_alias(NAME Twitch.Clients  NAMESPACE Gem TARGETS Gem::Twitch)
ly_create_alias(NAME Twitch.Tools    NAMESPACE Gem TARGETS Gem::Twitch)

