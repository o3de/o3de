#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

ly_add_target(
    NAME Microphone.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        microphone_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AudioSystem.Static
        PUBLIC
            3rdParty::libsamplerate
            Legacy::CryCommon
    COMPILE_DEFINITIONS
        PUBLIC
            USE_LIBSAMPLERATE
)

ly_add_target(
    NAME Microphone ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        microphone_shared_files.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Microphone.Static
)

ly_create_alias(NAME Microphone.Clients  NAMESPACE Gem TARGETS Gem::Microphone Gem::AudioSystem)
ly_create_alias(NAME Microphone.Tools    NAMESPACE Gem TARGETS Gem::Microphone Gem::AudioSystem.Editor)
