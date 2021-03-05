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

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if (NOT PAL_TRAIT_BUILD_NEWSBUILDER_SUPPORTED OR NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME NewsBuilder EXECUTABLE
    NAMESPACE Legacy
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        news_builder_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Widgets
            3rdParty::AWSNativeSDK::Dependencies
            3rdParty::AWSNativeSDK::Core
            3rdParty::AWSNativeSDK::S3
            Legacy::NewsShared
            AZ::AzCore
            AZ::AzQtComponents
            AZ::AzFramework
)
