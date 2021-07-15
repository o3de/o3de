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
        # Restricted fields can never be a requirement so no warning is issued
        return()
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
        # The object name is used to resolve ambiguities when a PAL directory is requested from
        # two different external subdirectory root paths
        # Such as if a PAL directory for two root object paths with same relative structure was requested to be Palified
        # i.e <gem-root1>/Platform/<platform-name>/IO and <gem-root2>/Platform/<platform-name>/IO
        # Normally the restricted PAL path for both gems would be "<restricted-root-path>/<platform-name>/IO".
        # The object name can be used to make this path unique
        # "<restricted-root-path>/<platform-name>/<custom-name1>/IO" for gem 1 and
        # "<restricted-root-path>/<platform-name>/<custom-name2>/IO" for gem 2
        set(object_name ${ARGV4})
    endif()

    # The Default object path for path is the LY_ROOT_FOLDER
    cmake_path(SET object_path NORMALIZE "${LY_ROOT_FOLDER}")
    if(${ARGC} GREATER 3)
        # The user has supplied an object restricted path, the object path for consideration
        cmake_path(SET object_path NORMALIZE ${ARGV3})
    endif()

    # The Default restricted object path is the result of the read_engine_restricted_path function
    cmake_path(SET object_restricted_path NORMALIZE "${O3DE_ENGINE_RESTRICTED_PATH}")
    if(${ARGC} GREATER 2)
        # The user has supplied an object restricted path
        cmake_path(SET object_restricted_path NORMALIZE ${ARGV2})
    endif()

    # The input path must exist in order to form a PAL path
    if (NOT EXISTS ${full_name})
        # if the file is not in the object path then we cannot determine a PAL file for it
        cmake_path(IS_PREFIX object_path ${full_name} is_input_path_in_root)
        if(is_input_path_in_root)
            cmake_path(RELATIVE_PATH full_name BASE_DIRECTORY ${object_path} OUTPUT_VARIABLE relative_object_path)
            cmake_path(SET current_object_path ${relative_object_path})

            # Remove one path segment from the end of the current_object_path and prepend it to the list path_segments
            cmake_path(GET current_object_path PARENT_PATH parent_path)
            cmake_path(GET current_object_path FILENAME path_segment)
            list(PREPEND path_segments_visited path_segment)
            cmake_path(COMPARE current_object_path NOT_EQUAL parent_path is_prev_path_segment)
            cmake_path(SET current_object_path "${parent_path}")

            while(is_prev_path_segment)
                # The Path is in a PAL structure
                # Decompose the path into sections before "Platform" and after "Platform"
                if(path_segment STREQUAL "Platform")
                    # The first path segment after the "<pre-platform-paths>/Platform/<post-platform-paths>"
                    # is a potential platform name. Store it off for later checks
                    list(GET path_segments_visited 0 candidate_platform_name)

                    # Store off all the path segments iterated from the end in the post-"Platform" path
                    cmake_path(APPEND post_platform_paths ${path_segments_visited})
                    # The parent path is just the pre-"Platform" section of the path
                    cmake_path(SET pre_platform_paths "${parent_path}")
                    break()
                endif()

                # Remove one path segment from the end of the current_object_path and prepend it to the list path_segments
                cmake_path(GET current_object_path PARENT_PATH parent_path)
                cmake_path(GET current_object_path FILENAME path_segment)
                list(PREPEND path_segments_visited path_segment)
                cmake_path(COMPARE current_object_path NOT_EQUAL parent_path is_prev_path_segment)
                cmake_path(SET current_object_path "${parent_path}")
            endwhile()

            # Compose a candidate restricted path and examine if it exists
            cmake_path(APPEND object_restricted_path ${pre_platform_paths} "Platform" ${post_platform_paths}
                OUTPUT_VARIABLE candidate_PAL_path)
            if(NOT EXISTS ${candidate_PAL_path})
                if("${candidate_platform_name}" IN_LIST PAL_RESTRICTED_PLATFORMS)
                    cmake_path(APPEND object_restricted_path ${candidate_platform_name} ${object_name}
                        ${pre_platform_paths} ${post_platform_paths} OUTPUT_VARIABLE candidate_PAL_path)
                endif()
            endif()
            if(EXISTS ${candidate_PAL_path})
                cmake_path(SET full_name ${candidate_PAL_path})
            endif()
        endif()
    endif()
    set(${out_name} ${full_name} PARENT_SCOPE)
endfunction()

function(ly_get_list_relative_pal_filename out_name in_name)
    ly_get_absolute_pal_filename(abs_name ${in_name} ${ARGN})
    cmake_path(RELATIVE_PATH abs_name BASE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE relative_name)
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
