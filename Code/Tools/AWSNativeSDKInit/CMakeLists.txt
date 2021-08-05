#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/source/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME AWSNativeSDKInit STATIC
    NAMESPACE AZ
    FILES_CMAKE
        aws_native_sdk_init_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            include
        PRIVATE
            ${pal_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::AWSNativeSDK::Core
            AZ::AzCore
)
