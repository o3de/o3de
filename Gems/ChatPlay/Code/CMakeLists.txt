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

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME ChatPlay.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        chatplay_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::AWSNativeSDK::Core
            3rdParty::dyad
            Gem::HttpRequestor
            Legacy::CryNetwork.Static
)

ly_add_target(
    NAME ChatPlay ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.ChatPlay.bfbc60c63ffd4b00927003735b26ce99.v0.1.0
    FILES_CMAKE
        chatplay_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::ChatPlay.Static
)
