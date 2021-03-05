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

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})
set(sdkwrapper_dir ${CMAKE_CURRENT_LIST_DIR}/../SDKWrapper)
set(sdkwrapper_pal_dir ${sdkwrapper_dir}/Platform/${PAL_PLATFORM_NAME})

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME FbxSDKWrapper STATIC
    NAMESPACE AZ
    FILES_CMAKE
        fbxsdkwrapper_files.cmake
        ${sdkwrapper_dir}/sdkwrapper_files.cmake
        ${sdkwrapper_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ../..
            ${sdkwrapper_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzToolsFramework
        PUBLIC
            3rdParty::FbxSdk
)
