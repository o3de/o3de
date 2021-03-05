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
ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)
ly_get_pal_tool_dirs(pal_tool_dirs ${CMAKE_CURRENT_LIST_DIR}/Platform)
ly_get_pal_tool_dirs(pal_tool_core_server_dirs ${CMAKE_CURRENT_LIST_DIR}/Core/Server/Platform)

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_BUILD_CRYSCOMPILESERVER_SUPPORTED OR NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

set(platform_tools_files)
foreach(enabled_platform ${LY_PAL_TOOLS_ENABLED})
    string(TOLOWER ${enabled_platform} enabled_platform_lowercase)
    ly_get_list_relative_pal_filename(pal_tool_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${enabled_platform})
    list(APPEND platform_tools_files ${pal_tool_dir}/pal_tools_${enabled_platform_lowercase}.cmake)
endforeach()

ly_add_target(
    NAME CrySCompileServer EXECUTABLE
    NAMESPACE Legacy
    FILES_CMAKE
        cryscompileserver_files.cmake
    PLATFORM_INCLUDE_FILES
        Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${platform_tools_files}
        ${common_dir}/${PAL_TRAIT_COMPILER_ID}/cryscompileserver_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            External
        PRIVATE
            ${pal_tool_dirs}
            ${pal_tool_core_server_dirs}
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::zlib
            AZ::AzCore
            AZ::AzFramework
)
ly_add_source_properties(
    SOURCES
        Core/Server/CrySimpleJobCompile.cpp
        CrySCompileServer.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)
ly_add_source_properties(
    SOURCES Core/Server/CrySimpleServer.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)
