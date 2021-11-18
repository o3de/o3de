#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_pal_tool_dirs(pal_tool_dirs ${CMAKE_CURRENT_LIST_DIR}/AssetBuilderSDK/Platform)

set(pal_files "")
foreach(enabled_platform ${LY_PAL_TOOLS_ENABLED})
    string(TOLOWER ${enabled_platform} enabled_platform_lowercase)
    ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/AssetBuilderSDK/Platform/${enabled_platform})
    list(APPEND pal_files ${pal_dir}/assetbuildersdk_${enabled_platform_lowercase}_files.cmake)
endforeach()

ly_add_target(
    NAME AssetBuilderSDK STATIC
    NAMESPACE AZ
    FILES_CMAKE
        assetbuilder_files.cmake
        ${pal_files}
    INCLUDE_DIRECTORIES
        PUBLIC
            .
        PRIVATE
            ${pal_tool_dirs}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
        PUBLIC
            AZ::AzFramework
            AZ::AzToolsFramework
            3rdParty::xxhash
)
ly_add_source_properties(
    SOURCES AssetBuilderSDK/AssetBuilderSDK.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)
