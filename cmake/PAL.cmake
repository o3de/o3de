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
file(GLOB detection_files ${o3de_engine_restricted_path}/*/cmake/PALDetection_*.cmake)
foreach(detection_file ${detection_files})
    include(${detection_file})
endforeach()

ly_set(PAL_PLATFORM_NAME ${LY_PLATFORM_DETECTION_${CMAKE_SYSTEM_NAME}})
string(TOLOWER ${PAL_PLATFORM_NAME} PAL_PLATFORM_NAME_LOWERCASE)
ly_set(PAL_PLATFORM_NAME_LOWERCASE, ${PAL_PLATFORM_NAME_LOWERCASE})

ly_set(PAL_HOST_PLATFORM_NAME ${LY_HOST_PLATFORM_DETECTION_${CMAKE_SYSTEM_NAME}})
string(TOLOWER ${PAL_HOST_PLATFORM_NAME} PAL_HOST_PLATFORM_NAME_LOWERCASE)
ly_set(PAL_HOST_PLATFORM_NAME_LOWERCASE ${PAL_HOST_PLATFORM_NAME_LOWERCASE})

set(PAL_RESTRICTED_PLATFORMS)

string(LENGTH ${o3de_engine_restricted_path} engine_restricted_length)
file(GLOB pal_restricted_files ${o3de_engine_restricted_path}/*/cmake/PAL_*.cmake)
foreach(pal_restricted_file ${pal_restricted_files})
    string(FIND ${pal_restricted_file} "/cmake/PAL" end)
    if(${end} GREATER -1)
        math(EXPR platform_length "${end} - ${engine_restricted_length} - 1")
        math(EXPR platform_start "${engine_restricted_length} + 1")
        string(SUBSTRING ${pal_restricted_file} ${platform_start} ${platform_length} platform)
        list(APPEND PAL_RESTRICTED_PLATFORMS "${platform}")
    endif()
endforeach()
ly_set(PAL_RESTRICTED_PLATFORMS ${PAL_RESTRICTED_PLATFORMS})

function(ly_get_absolute_pal_filename out_name in_name)
    set(full_name ${in_name})

    if(${ARGC} GREATER 4)
        # The user has supplied an object restricted path, the object path and the object name for consideration
        set(object_restricted_path ${ARGV2})
        set(object_path ${ARGV3})
        set(object_name ${ARGV4})

        # if the file is not in the object path then we cannot determine a PAL file for it
        string(FIND ${full_name} ${object_path}  pos)
        if(${pos} GREATER -1)
            if (NOT EXISTS ${full_name})
                string(REGEX MATCH "${object_path}/(.*)/Platform/([^/]*)/?(.*)$" match ${full_name})
                if("${CMAKE_MATCH_1}" STREQUAL "")
                    string(REGEX MATCH "${object_path}/Platform/([^/]*)/?(.*)$" match ${full_name})
                    set(full_name ${object_restricted_path}/${CMAKE_MATCH_1}/${object_name})
                    if(NOT "${CMAKE_MATCH_2}" STREQUAL "")
                        string(APPEND full_name "/" ${CMAKE_MATCH_2})
                    endif()
                elseif("${CMAKE_MATCH_2}" IN_LIST PAL_RESTRICTED_PLATFORMS)
                    set(full_name ${object_restricted_path}/${CMAKE_MATCH_2}/${object_name}/${CMAKE_MATCH_1})
                    if(NOT "${CMAKE_MATCH_3}" STREQUAL "")
                        string(APPEND full_name "/" ${CMAKE_MATCH_3})
                    endif()
                endif()
            endif()
        endif()
        set(${out_name} ${full_name} PARENT_SCOPE)

    elseif(${ARGC} GREATER 3)
        # The user has supplied an object restricted path, the object path for consideration
        set(object_restricted_path ${ARGV2})
        set(object_path ${ARGV3})

        # if the file is not in the object path then we cannot determine a PAL file for it
        string(FIND ${full_name} ${object_path} pos)
        if(${pos} GREATER -1)
            if (NOT EXISTS ${full_name})
                string(REGEX MATCH "${object_path}/(.*)/Platform/([^/]*)/?(.*)$" match ${full_name})
                if("${CMAKE_MATCH_1}" STREQUAL "")
                    string(REGEX MATCH "${object_path}/Platform/([^/]*)/?(.*)$" match ${full_name})
                    set(full_name ${object_restricted_path}/${CMAKE_MATCH_1})
                    if(NOT "${CMAKE_MATCH_2}" STREQUAL "")
                        string(APPEND full_name "/" ${CMAKE_MATCH_2})
                    endif()
                elseif("${CMAKE_MATCH_2}" IN_LIST PAL_RESTRICTED_PLATFORMS)
                    set(full_name ${object_restricted_path}/${CMAKE_MATCH_2}/${CMAKE_MATCH_1})
                    if(NOT "${CMAKE_MATCH_3}" STREQUAL "")
                        string(APPEND full_name "/" ${CMAKE_MATCH_3})
                    endif()
                endif()
            endif()
        endif()
        set(${out_name} ${full_name} PARENT_SCOPE)

    else()
        # The user has not supplied any path so we must assume it is the o3de engine restricted and o3de engine path
        # if the file is not in the o3de engine path then we cannot determine a PAL file for it
        string(FIND ${full_name} ${o3de_engine_path} pos)
        if(${pos} GREATER -1)
            if (NOT EXISTS ${full_name})
                string(REGEX MATCH "${o3de_engine_path}/(.*)/Platform/([^/]*)/?(.*)$" match ${full_name})
                if("${CMAKE_MATCH_1}" STREQUAL "")
                    string(REGEX MATCH "${o3de_engine_path}/Platform/([^/]*)/?(.*)$" match ${full_name})
                    set(full_name ${o3de_engine_restricted_path}/${CMAKE_MATCH_1})
                    if(NOT "${CMAKE_MATCH_2}" STREQUAL "")
                        string(APPEND full_name "/" ${CMAKE_MATCH_2})
                    endif()
                elseif("${CMAKE_MATCH_2}" IN_LIST PAL_RESTRICTED_PLATFORMS)
                    set(full_name ${o3de_engine_restricted_path}/${CMAKE_MATCH_2}/${CMAKE_MATCH_1})
                    if(NOT "${CMAKE_MATCH_3}" STREQUAL "")
                        string(APPEND full_name "/" ${CMAKE_MATCH_3})
                    endif()
                endif()
            endif()
        endif()
        set(${out_name} ${full_name} PARENT_SCOPE)
    endif()
    if (NOT ${in_name} STREQUAL ${full_name})
    endif()
endfunction()

function(ly_get_list_relative_pal_filename out_name in_name)
    ly_get_absolute_pal_filename(abs_name ${in_name} ${ARGN})
    file(RELATIVE_PATH relative_name ${CMAKE_CURRENT_LIST_DIR} ${abs_name})
    set(${out_name} ${relative_name} PARENT_SCOPE)
endfunction()

ly_get_absolute_pal_filename(pal_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Platform/${PAL_PLATFORM_NAME})

ly_include_cmake_file_list(${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake)

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
include(${pal_dir}/Toolchain_${PAL_PLATFORM_NAME_LOWERCASE}.cmake OPTIONAL)

set(LY_DISABLE_TEST_MODULES FALSE CACHE BOOL "Option to forcibly disable the inclusion of test targets in the build")

if(LY_DISABLE_TEST_MODULES)
    ly_set(PAL_TRAIT_BUILD_TESTS_SUPPORTED FALSE)
endif()
