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

# PAL allows us to deal with platforms in a flexible way. In order to do that, we need
# to be able to refer to the current platform in a generic way.
# This cmake file provides variables and configurations for the current platform
#
# Variables:
# PAL_PLATFORM_NAME: name of the platform (regular usage)
# PAL_PLATFORM_NAME_LOWERCASE: name of the platform in lower case (part of filenames)
#

file(GLOB detection_files "cmake/Platform/*/PALDetection_*.cmake")
foreach(detection_file ${detection_files})
    include(${detection_file})
endforeach()
file(GLOB detection_files "restricted/*/cmake/PALDetection_*.cmake")
foreach(detection_file ${detection_files})
    include(${detection_file})
endforeach()

set(PAL_PLATFORM_NAME ${LY_PLATFORM_DETECTION_${CMAKE_SYSTEM_NAME}})
string(TOLOWER ${PAL_PLATFORM_NAME} PAL_PLATFORM_NAME_LOWERCASE)

set(PAL_HOST_PLATFORM_NAME ${LY_HOST_PLATFORM_DETECTION_${CMAKE_SYSTEM_NAME}})
string(TOLOWER ${PAL_HOST_PLATFORM_NAME} PAL_HOST_PLATFORM_NAME_LOWERCASE)

set(PAL_RESTRICTED_PLATFORMS)
file(GLOB pal_restricted_files "restricted/*/cmake/PAL_*.cmake")
foreach(pal_restricted_file ${pal_restricted_files})
    string(FIND ${pal_restricted_file} "restricted/" start)
    math(EXPR start "${start} + 11")
    string(FIND ${pal_restricted_file} "/cmake/PAL" end)
    if(${end} GREATER -1)
        math(EXPR length "${end} - ${start}")
        string(SUBSTRING ${pal_restricted_file} ${start} ${length} platform)
        list(APPEND PAL_RESTRICTED_PLATFORMS "${platform}")
    endif()
endforeach()

function(ly_get_absolute_pal_filename out_name in_name)
    if(${ARGC} GREATER 2)
        set(repo_dir ${ARGV2})
    else()
        if(DEFINED LY_ROOT_FOLDER)
            set(repo_dir ${LY_ROOT_FOLDER})
        else()
            set(repo_dir ${CMAKE_CURRENT_SOURCE_DIR})
        endif()
    endif()
    set(full_name ${in_name})
    string(REGEX MATCH "${repo_dir}/(.*)/Platform/([^/]*)(.*)?" match ${full_name})
    if(${CMAKE_MATCH_2} IN_LIST PAL_RESTRICTED_PLATFORMS)
        set(full_name ${repo_dir}/restricted/${CMAKE_MATCH_2}/${CMAKE_MATCH_1})
        if(NOT "${CMAKE_MATCH_3}" STREQUAL "")
            string(APPEND full_name ${CMAKE_MATCH_3})
        endif()
    endif()
    set(${out_name} ${full_name} PARENT_SCOPE)
endfunction()

function(ly_get_list_relative_pal_filename out_name in_name)
    if(${ARGC} GREATER 2)
        ly_get_absolute_pal_filename(abs_name ${in_name} ${ARGV2})
    else()
        ly_get_absolute_pal_filename(abs_name ${in_name})
    endif()
    file(RELATIVE_PATH relative_name ${CMAKE_CURRENT_LIST_DIR} ${abs_name})
    set(${out_name} ${relative_name} PARENT_SCOPE)
endfunction()

ly_get_absolute_pal_filename(pal_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Platform/${PAL_PLATFORM_NAME})

ly_include_cmake_file_list(${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake)

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
include(${pal_dir}/Toolchain_${PAL_PLATFORM_NAME_LOWERCASE}.cmake OPTIONAL)
