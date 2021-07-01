#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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


#! o3de_restricted_id: Reads the "restricted" key from the o3de manifest
#
# \arg:o3de_json_file name of the o3de json file to read the "restricted_name" key from
# \arg:restricted returns the restricted association element from an o3de json, otherwise engine 'o3de' is assumed
# \arg:o3de_json_file name of the o3de json file
function(o3de_restricted_id o3de_json_file restricted)
    ly_file_read(${o3de_json_file} json_data)
    string(JSON restricted_entry ERROR_VARIABLE json_error GET ${json_data} "restricted_name")
    if(json_error)
        message(WARNING "Unable to read restricted from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(restricted_entry)
       set(${restricted} ${restricted_entry} PARENT_SCOPE)
    endif()
endfunction()

#! o3de_find_restricted_folder:
#
# \arg:restricted_path returns the path of the o3de restricted folder with name restricted_name
# \arg:restricted_name name of the restricted
function(o3de_find_restricted_folder restricted_name restricted_path)
    # Read the restricted path from engine.json if one EXISTS
    ly_file_read(${LY_ROOT_FOLDER}/engine.json engine_json_data)
    string(JSON restricted_subdirs_count ERROR_VARIABLE engine_json_error LENGTH ${engine_json_data} "restricted")
    if(restricted_subdirs_count GREATER 0)
        string(JSON restricted_subdir ERROR_VARIABLE engine_json_error GET ${engine_json_data} "restricted" "0")
        set(${restricted_path} ${restricted_subdir} PARENT_SCOPE)
        return()
    endif()


    file(TO_CMAKE_PATH "$ENV{USERPROFILE}" home_directory) # Windows
    if(NOT EXISTS ${home_directory})
        file(TO_CMAKE_PATH "$ENV{HOME}" home_directory) # Unix
        if (NOT EXISTS ${home_directory})
            return()
        endif()
    endif()

    # Examine the o3de manifest file for the list of restricted directories
    set(o3de_manifest_path ${home_directory}/.o3de/o3de_manifest.json)
    if(EXISTS ${o3de_manifest_path})
        ly_file_read(${o3de_manifest_path} o3de_manifest_json_data)
        string(JSON restricted_subdirs_count ERROR_VARIABLE engine_json_error LENGTH ${o3de_manifest_json_data} "restricted")
        if(restricted_subdirs_count GREATER 0)
            math(EXPR restricted_subdirs_range "${restricted_subdirs_count}-1")
            foreach(restricted_subdir_index RANGE ${restricted_subdirs_range})
                string(JSON restricted_subdir ERROR_VARIABLE engine_json_error GET ${o3de_manifest_json_data} "restricted" "${restricted_subdir_index}")
                list(APPEND restricted_subdirs ${restricted_subdir})
            endforeach()
        endif()
    endif()
    # Iterate over the restricted directories from the manifest file
    foreach(restricted_entry ${restricted_subdirs})
        set(restricted_json_file ${restricted_entry}/restricted.json)
        ly_file_read(${restricted_json_file} restricted_json)
        string(JSON this_restricted_name ERROR_VARIABLE json_error GET ${restricted_json} "restricted_name")
        if(json_error)
            message(WARNING "Unable to read restricted_name from '${restricted_json_file}', error: ${json_error}")
        else()
            if(this_restricted_name STREQUAL restricted_name)
                set(${restricted_path} ${restricted_entry} PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
endfunction()


#! o3de_restricted_path:
#
# \arg:o3de_json_file json file to read restricted id from
# \arg:restricted_name name of the restricted object
function(o3de_restricted_path o3de_json_file restricted_path)
    o3de_restricted_id(${o3de_json_file} restricted_name)
    if(restricted_name)
        o3de_find_restricted_folder(${restricted_name} restricted_folder)
        if(restricted_folder)
            set(${restricted_path} ${restricted_folder} PARENT_SCOPE)
        endif()
    endif()
endfunction()

#! read_engine_restricted_path: Locates the restricted path within the engine from a json file
#
# \arg:output_restricted_path returns the path of the o3de restricted folder with name restricted_name
function(read_engine_restricted_path output_restricted_path)
    # Set manifest path to path in the user home directory
    set(manifest_path ${LY_ROOT_FOLDER}/engine.json)
    if(EXISTS ${manifest_path})
        o3de_restricted_path(${manifest_path} output_restricted_path)
    endif()
endfunction()

read_engine_restricted_path(O3DE_ENGINE_RESTRICTED_PATH)

file(GLOB detection_files ${O3DE_ENGINE_RESTRICTED_PATH}/*/cmake/PALDetection_*.cmake)
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

string(LENGTH "${O3DE_ENGINE_RESTRICTED_PATH}" engine_restricted_length)
file(GLOB pal_restricted_files ${O3DE_ENGINE_RESTRICTED_PATH}/*/cmake/PAL_*.cmake)
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
        file(RELATIVE_PATH relative_path ${object_path} ${full_name})
        if (NOT (IS_ABSOLUTE relative_path OR relative_path MATCHES [[^(\.\./)+(.*)]]))
            if (NOT EXISTS ${full_name})
                string(REGEX MATCH "${object_path}/(.*)/Platform/([^/]*)/?(.*)$" match ${full_name})
                if(NOT CMAKE_MATCH_1)
                    string(REGEX MATCH "${object_path}/Platform/([^/]*)/?(.*)$" match ${full_name})
                    set(full_name ${object_restricted_path}/${CMAKE_MATCH_1}/${object_name})
                    if(CMAKE_MATCH_2)
                        string(APPEND full_name "/" ${CMAKE_MATCH_2})
                    endif()
                elseif("${CMAKE_MATCH_2}" IN_LIST PAL_RESTRICTED_PLATFORMS)
                    set(full_name ${object_restricted_path}/${CMAKE_MATCH_2}/${object_name}/${CMAKE_MATCH_1})
                    if(CMAKE_MATCH_3)
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
        file(RELATIVE_PATH relative_path ${object_path} ${full_name})
        if (NOT (IS_ABSOLUTE relative_path OR relative_path MATCHES [[^(\.\./)+(.*)]]))
            if (NOT EXISTS ${full_name})
                string(REGEX MATCH "${object_path}/(.*)/Platform/([^/]*)/?(.*)$" match ${full_name})
                if(NOT CMAKE_MATCH_1)
                    string(REGEX MATCH "${object_path}/Platform/([^/]*)/?(.*)$" match ${full_name})
                    set(full_name ${object_restricted_path}/${CMAKE_MATCH_1})
                    if(CMAKE_MATCH_2)
                        string(APPEND full_name "/" ${CMAKE_MATCH_2})
                    endif()
                elseif("${CMAKE_MATCH_2}" IN_LIST PAL_RESTRICTED_PLATFORMS)
                    set(full_name ${object_restricted_path}/${CMAKE_MATCH_2}/${CMAKE_MATCH_1})
                    if(CMAKE_MATCH_3)
                        string(APPEND full_name "/" ${CMAKE_MATCH_3})
                    endif()
                endif()
            endif()
        endif()
        set(${out_name} ${full_name} PARENT_SCOPE)

    else()
        # The user has not supplied any path so we must assume it is the o3de engine restricted and o3de engine path
        # if the file is not in the o3de engine path then we cannot determine a PAL file for it
        file(RELATIVE_PATH relative_path ${LY_ROOT_FOLDER} ${full_name})
        if (NOT (IS_ABSOLUTE relative_path OR relative_path MATCHES [[^(\.\./)+(.*)]]))
            if (NOT EXISTS ${full_name})
                string(REGEX MATCH "${LY_ROOT_FOLDER}/(.*)/Platform/([^/]*)/?(.*)$" match ${full_name})
                if(NOT CMAKE_MATCH_1)
                    string(REGEX MATCH "${LY_ROOT_FOLDER}/Platform/([^/]*)/?(.*)$" match ${full_name})
                    set(full_name ${O3DE_ENGINE_RESTRICTED_PATH}/${CMAKE_MATCH_1})
                    if(CMAKE_MATCH_2)
                        string(APPEND full_name "/" ${CMAKE_MATCH_2})
                    endif()
                elseif("${CMAKE_MATCH_2}" IN_LIST PAL_RESTRICTED_PLATFORMS)
                    set(full_name ${O3DE_ENGINE_RESTRICTED_PATH}/${CMAKE_MATCH_2}/${CMAKE_MATCH_1})
                    if(CMAKE_MATCH_3)
                        string(APPEND full_name "/" ${CMAKE_MATCH_3})
                    endif()
                endif()
            endif()
        endif()
        set(${out_name} ${full_name} PARENT_SCOPE)
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
