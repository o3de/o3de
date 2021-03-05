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

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_get_pal_tool_dirs(pal_tool_dirs ${CMAKE_CURRENT_LIST_DIR}/Platform)
 
set(pal_files "")
set(pal_cmake_files "")
foreach(enabled_platform ${LY_PAL_TOOLS_ENABLED})
    string(TOLOWER ${enabled_platform} enabled_platform_lowercase)
    ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${enabled_platform})
    list(APPEND pal_files ${pal_dir}/platform_${enabled_platform_lowercase}_files.cmake)
    list(APPEND pal_cmake_files ${pal_dir}/platform_${enabled_platform_lowercase}.cmake)
endforeach()

ly_add_target(
    NAME ShaderCacheGen EXECUTABLE
    NAMESPACE Legacy
    FILES_CMAKE
        shader_cache_gen_files.cmake
        ${pal_files}
    PLATFORM_INCLUDE_FILES
        ${pal_cmake_files}
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            ${pal_tool_dirs}
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
)
ly_add_source_properties(
    SOURCES ShaderCacheGen.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)
