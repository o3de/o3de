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

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME Microphone.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        microphone_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
        PUBLIC
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AudioSystem
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
    RUNTIME_DEPENDENCIES
        Gem::AudioSystem
)
