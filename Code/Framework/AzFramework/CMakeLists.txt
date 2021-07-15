#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)

set(LY_ENABLE_STATISTICAL_PROFILING OFF CACHE BOOL "Enables statistical profiling when using AZ_PROFILE_SCOPE. If True, it takes effect only if RAD Telemetry is disabled.")
set(LY_TOUCHBENDING_LAYER_BIT 63 CACHE STRING "Use TouchBending as the collision layer. The TouchBending layer can be a number from 1 to 63 (Default=63).")

ly_add_target(
    NAME AzFramework STATIC
    NAMESPACE AZ
    FILES_CMAKE
        AzFramework/azframework_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
        PRIVATE
            ${common_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
        PUBLIC
            AZ::GridMate
            3rdParty::md5
            3rdParty::zlib
            3rdParty::zstd
            3rdParty::lz4
)

if(LY_ENABLE_STATISTICAL_PROFILING)
    ly_add_source_properties(
        SOURCES AzFramework/Debug/StatisticalProfilerProxy.h
        PROPERTY COMPILE_DEFINITIONS 
        VALUES AZ_STATISTICAL_PROFILING_ENABLED
    )
endif()

ly_add_source_properties(
    SOURCES 
        AzFramework/Physics/Collision/CollisionGroups.cpp
        AzFramework/Physics/Collision/CollisionLayers.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES TOUCHBENDING_LAYER_BIT=${LY_TOUCHBENDING_LAYER_BIT}
)
