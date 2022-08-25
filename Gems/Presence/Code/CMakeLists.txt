#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

ly_add_target(
    NAME Presence.Headers HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        presence_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
            AZ::AzCore
            AZ::AzFramework
)

ly_add_target(
    NAME Presence ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    FILES_CMAKE
        presence_shared_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::Presence.Headers
)

# we activate the presence gem (if enabled) only on client applications such as the launcher:
ly_create_alias(NAME Presence.Clients NAMESPACE Gem TARGETS Gem::Presence)
