#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

#! o3de_get_home_path: returns the home path
#
# \arg:o3de_manifest_path returns the path of the manifest
function(o3de_get_home_path o3de_home_path)
    # The o3de_manifest.json is in the home directory / .o3de folder
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}" home_path) # Windows
    if(NOT EXISTS ${home_path})
        file(TO_CMAKE_PATH "$ENV{HOME}" home_path) # Unix
        if (NOT EXISTS ${home_path})
            message(FATAL_ERROR "o3de Home path not found")
        endif()
    endif()
    set(${o3de_home_path} ${home_path} PARENT_SCOPE)
endfunction()

#! o3de_get_manifest_path: returns the path to the manifest
#
# \arg:o3de_manifest_path returns the path of the manifest
function(o3de_get_manifest_path o3de_manifest_path)
    # The o3de_manifest.json is in the home directory / .o3de folder
    o3de_get_home_path(o3de_home_path)
    set(${o3de_manifest_path} ${o3de_home_path}/.o3de/o3de_manifest.json PARENT_SCOPE)
endfunction()

#! o3de_read_manifest: returns the contents of the manifest
#
# \arg:restricted_subdirs returns the restricted elements from the manifest
function(o3de_read_manifest o3de_manifest_json_data)
    #get the manifest path
    o3de_get_manifest_path(o3de_manifest_path)
    if(EXISTS ${o3de_manifest_path})
        ly_file_read(${o3de_manifest_path} json_data)
        set(${o3de_manifest_json_data} ${json_data} PARENT_SCOPE)
    endif()
endfunction()

#! o3de_find_gem_with_registered_external_subdirs: Query the path of a gem using its name
#  IMPORTANT NOTE: This does not take into account any gem versions or dependency resolution, 
#  which is fine if you don't need it and just want speed.
# \arg:gem_name the gem name to find
# \arg:output_gem_path the path of the gem to set
# \arg:registered_external_subdirs a list of external subdirectories registered accross
#      all manifest files to look for gems
function(o3de_find_gem_with_registered_external_subdirs gem_name output_gem_path registered_external_subdirs)
    foreach(external_subdir IN LISTS registered_external_subdirs)
        set(candidate_gem_path ${external_subdir}/gem.json)
        if(EXISTS ${candidate_gem_path})
            o3de_read_json_key(gem_json_name ${candidate_gem_path} "gem_name")
            if(gem_json_name STREQUAL gem_name)
                set(${output_gem_path} ${external_subdir} PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
endfunction()

#! o3de_find_gem: Query the path of a gem using its name
#
# \arg:gem_name the gem name to find
# \arg:output_gem_path the path of the gem to set
#
# If the list of registered external subdirectories are available in the caller,
# then slightly better more performance can be achieved by calling `o3de_find_gem_with_registered_external_subdirs` above
function(o3de_find_gem gem_name output_gem_path)
    get_all_external_subdirectories(registered_external_subdirs)
    o3de_find_gem_with_registered_external_subdirs(${gem_name} gem_path "${registered_external_subdirs}")
    set(${output_gem_path} ${gem_path} PARENT_SCOPE)
endfunction()

#! o3de_manifest_restricted: returns the manifests restricted paths
#
# \arg:restricted returns the restricted elements from the manifest
function(o3de_manifest_restricted restricted)
    #read the manifest
    o3de_read_manifest(o3de_manifest_json_data)
    string(JSON restricted_count ERROR_VARIABLE json_error LENGTH ${o3de_manifest_json_data} "restricted")
    if(json_error)
        # Restricted fields can never be a requirement so no warning is issued
        return()
    endif()
    if(restricted_count GREATER 0)
        math(EXPR restricted_range "${restricted_count}-1")
        foreach(restricted_index RANGE ${restricted_range})
            string(JSON restricted_entry ERROR_VARIABLE json_error GET ${o3de_manifest_json_data} "restricted" "${restricted_index}")
            list(APPEND restricted_entries ${restricted_entry})
        endforeach()
    endif()
    set(${restricted} ${restricted_entries} PARENT_SCOPE)
endfunction()

#! o3de_json_restricted: returns the restricted element from a json
#
# \arg:restricted returns the restricted element of the json
function(o3de_json_restricted json_path restricted)
    if(EXISTS ${json_path})
        ly_file_read(${json_path} json_data)
        string(JSON restricted_entry ERROR_VARIABLE json_error GET ${json_data} "restricted")
        if(json_error)
            # Restricted fields can never be a requirement so no warning is issued
            return()
        endif()
        set(${restricted} ${restricted_entry} PARENT_SCOPE)
    endif()
endfunction()

#! o3de_restricted_id: determines the restricted object for this json
#
# Find this objects restricted name. If the object has a "restricted" element
# If it does not have one it inherits its parents "restricted" element if it has one
# If the parent does not have one it inherits its parents parent "restricted" element is it has one and so on...
# We stop looking if the object or parent is in the manifest, as the manifest only has top level objects
# which means they have no children.
#
# \arg:o3de_json_file name of the o3de json file to read the "restricted" key from
# \arg:restricted returns the restricted association element from an o3de json, otherwise its doesnt change anything
# \arg:o3de_json_file name of the o3de json file
function(o3de_restricted_id o3de_json_file restricted parent_relative_path)
    # read the passed in o3de json and see if "restricted" is set
    o3de_json_restricted(${o3de_json_file} restricted_name)
    if(restricted_name)
        set(${parent_relative_path} "" PARENT_SCOPE)
        set(${restricted} ${restricted_name} PARENT_SCOPE)
        return()
    endif()

    # This object did not have a "restricted" set, now we must look at the parent
    # Stop if this is a top level object
    o3de_manifest_restricted(manifest_restricted_paths)
    cmake_path(GET o3de_json_file PARENT_PATH o3de_json_file_parent)
    cmake_path(GET o3de_json_file_parent FILENAME relative_path)
    cmake_path(GET o3de_json_file_parent PARENT_PATH o3de_json_file_parent)
    if(${o3de_json_file_parent} IN_LIST manifest_restricted_paths)
        set(${parent_relative_path} "" PARENT_SCOPE)
        set(${restricted} "" PARENT_SCOPE)
        return()
    endif()

    set(is_prev_path_segment TRUE)
    while(is_prev_path_segment)
        if(EXISTS ${o3de_json_file_parent}/engine.json)
            o3de_json_restricted(${o3de_json_file_parent}/engine.json restricted_name)
            if(restricted_name)
                set(${parent_relative_path} ${relative_path} PARENT_SCOPE)
                set(${restricted} ${restricted_name} PARENT_SCOPE)
                return()
            endif()
        endif()
        if(EXISTS ${o3de_json_file_parent}/project.json)
            o3de_json_restricted(${o3de_json_file_parent}/project.json restricted_name)
            if(restricted_name)
                set(${parent_relative_path} ${relative_path} PARENT_SCOPE)
                set(${restricted} ${restricted_name} PARENT_SCOPE)
                return()
            endif()
        endif()
        if(EXISTS ${o3de_json_file_parent}/gem.json)
            o3de_json_restricted(${o3de_json_file_parent}/gem.json restricted_name)
            if(restricted_name)
                set(${parent_relative_path} ${relative_path} PARENT_SCOPE)
                set(${restricted} ${restricted_name} PARENT_SCOPE)
                return()
            endif()
        endif()

        if(${o3de_json_file_parent} IN_LIST manifest_restricted_paths)
            set(${parent_relative_path} "" PARENT_SCOPE)
            set(${restricted} "" PARENT_SCOPE)
            return()
        endif()

        # Remove one path segment from the end of the o3de json candidate path
        cmake_path(GET o3de_json_file_parent PARENT_PATH parent_path)
        cmake_path(GET o3de_json_file_parent FILENAME path_segment)
        cmake_path(COMPARE "${o3de_json_file_parent}" NOT_EQUAL "${parent_path}" is_prev_path_segment)
        cmake_path(SET o3de_json_file_parent "${parent_path}")
        cmake_path(SET relative_path "${path_segment}/${relative_path}")
    endwhile()
endfunction()

#! o3de_find_restricted_folder:
#
# \arg:restricted_path returns the path of the o3de restricted folder using the restricted_name
# \arg:restricted_name name of the restricted
function(o3de_find_restricted_folder restricted_name restricted_path)
    o3de_manifest_restricted(restricted_entries)
    # Iterate over the restricted directories from the manifest file
    foreach(restricted_entry ${restricted_entries})
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
# \arg:restricted_path output path of the restricted object
# \arg:parent_relative_path optional output of the path relative to the parent
function(o3de_restricted_path o3de_json_file restricted_path) #parent_relative_path
    o3de_restricted_id(${o3de_json_file} restricted_name parent_relative)
    if(${ARGC} GREATER 2)
        set(${ARGV2} ${parent_relative} PARENT_SCOPE)
    endif()
    if(restricted_name)
        o3de_find_restricted_folder(${restricted_name} restricted_folder)
        if(restricted_folder)
            set(${restricted_path} ${restricted_folder} PARENT_SCOPE)
        else()
            get_filename_component(o3de_json_file_parent ${o3de_json_file} DIRECTORY)
            set(${restricted_path} ${o3de_json_file_parent}/restricted PARENT_SCOPE)
        endif()
    endif()
endfunction()

# detect open platforms
file(GLOB detection_files "cmake/Platform/*/PALDetection_*.cmake")
foreach(detection_file ${detection_files})
    include(${detection_file})
endforeach()

# set the O3DE_ENGINE_RESTRICTED_PATH
o3de_restricted_path(${LY_ROOT_FOLDER}/engine.json O3DE_ENGINE_RESTRICTED_PATH)

# detect platforms in the restricted path
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

# In addition to platform name, set the platform architecture if supported
if (LY_ARCHITECTURE_DETECTION_${PAL_PLATFORM_NAME})
    ly_set(LY_ARCHITECTURE_NAME_EXTENSION "_${LY_ARCHITECTURE_DETECTION_${PAL_PLATFORM_NAME}}")
endif()
if (LY_HOST_ARCHITECTURE_DETECTION_${PAL_HOST_PLATFORM_NAME})
    ly_set(LY_HOST_ARCHITECTURE_NAME_EXTENSION "_${LY_HOST_ARCHITECTURE_DETECTION_${PAL_HOST_PLATFORM_NAME}}")
endif()



set(PAL_RESTRICTED_PLATFORMS)

file(GLOB pal_restricted_files ${O3DE_ENGINE_RESTRICTED_PATH}/*/cmake/PAL_*.cmake)
foreach(pal_restricted_file ${pal_restricted_files})
    # Get relative path from restricted root directory
    cmake_path(RELATIVE_PATH pal_restricted_file BASE_DIRECTORY ${O3DE_ENGINE_RESTRICTED_PATH} OUTPUT_VARIABLE relative_pal_restricted_file)
    # Split relative restricted path into path segments
    string(REPLACE "/" ";" pal_restricted_segments ${relative_pal_restricted_file})
    # Retrieve the first path segment which should be the restricted platform
    list(GET pal_restricted_segments 0 platform)
    # Append the new restricted platform
    string(TOLOWER ${platform} platform_lower)
    list(APPEND PAL_RESTRICTED_PLATFORMS ${platform_lower})
endforeach()
list(REMOVE_DUPLICATES PAL_RESTRICTED_PLATFORMS)

ly_set(PAL_RESTRICTED_PLATFORMS ${PAL_RESTRICTED_PLATFORMS})

function(ly_get_absolute_pal_filename out_name in_name)
    message(DEPRECATION "ly_get_list_relative_pal_filename is being deprecated, change your code to use o3de_pal_dir instead.")

    # parent relative path is optional
    if(${ARGC} GREATER 4)
        if(ARGV4)
            set(parent_relative_path ${ARGV4})
        endif()
    endif()

    # The Default object path for path is the LY_ROOT_FOLDER
    cmake_path(SET object_path NORMALIZE "${LY_ROOT_FOLDER}")
    if(${ARGC} GREATER 3)
        if(ARGV3)
            # The user has supplied an object restricted path, the object path for consideration
            cmake_path(SET object_path NORMALIZE ${ARGV3})
        endif()
    endif()

    # The default restricted object path is O3DE_ENGINE_RESTRICTED_PATH
    cmake_path(SET object_restricted_path NORMALIZE "${O3DE_ENGINE_RESTRICTED_PATH}")
    if(${ARGC} GREATER 2)
        if(ARGV3)
            # The user has supplied an object restricted path
            cmake_path(SET object_restricted_path NORMALIZE ${ARGV2})
        endif()
    endif()

    if(${ARGC} GREATER 4)
        o3de_pal_dir(abs_name ${in_name} "${object_restricted_path}" "${object_path}" "${parent_relative_path}")
    else()
        o3de_pal_dir(abs_name ${in_name} "${object_restricted_path}" "${object_path}")
    endif()
    set(${out_name} ${abs_name} PARENT_SCOPE)
endfunction()

function(o3de_pal_dir out_name in_name object_restricted_path object_path) #parent_relative_path)
    set(full_name ${in_name})

    # parent relative path is optional
    if(${ARGC} GREATER 4)
        set(parent_relative_path ${ARGV4})
    endif()

    # The input path must not exist in order to form a restricted PAL path
    if (NOT EXISTS ${full_name})
        # if the file is not in the object path then we cannot determine a PAL file for it
        cmake_path(IS_PREFIX object_path ${full_name} is_input_path_in_root)
        if(is_input_path_in_root)
            cmake_path(RELATIVE_PATH full_name BASE_DIRECTORY ${object_path} OUTPUT_VARIABLE relative_object_path)
            cmake_path(SET current_object_path ${relative_object_path})

            # Remove one path segment from the end of the current_object_path and prepend it to the list path_segments
            cmake_path(GET current_object_path PARENT_PATH parent_path)
            cmake_path(GET current_object_path FILENAME path_segment)
            list(PREPEND path_segments_visited ${path_segment})
            cmake_path(COMPARE "${current_object_path}" NOT_EQUAL "${parent_path}" is_prev_path_segment)
            cmake_path(SET current_object_path "${parent_path}")

            set(is_prev_path_segment TRUE)
            while(is_prev_path_segment)
                # Remove one path segment from the end of the current_object_path and prepend it to the list path_segments
                cmake_path(GET current_object_path PARENT_PATH parent_path)
                cmake_path(GET current_object_path FILENAME path_segment)
                cmake_path(COMPARE "${current_object_path}" NOT_EQUAL "${parent_path}" is_prev_path_segment)
                cmake_path(SET current_object_path "${parent_path}")
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

                list(PREPEND path_segments_visited ${path_segment})
            endwhile()

            # Compose a candidate restricted path and examine if it exists
            cmake_path(APPEND ${pre_platform_paths} "Platform" ${post_platform_paths}
                OUTPUT_VARIABLE candidate_PAL_path)
            if(NOT EXISTS ${candidate_PAL_path})
                string(TOLOWER ${candidate_platform_name} candidate_platform_name_lower)
                if("${candidate_platform_name_lower}" IN_LIST PAL_RESTRICTED_PLATFORMS)
                    cmake_path(APPEND object_restricted_path ${candidate_platform_name} ${parent_relative_path}
                        ${pre_platform_paths} OUTPUT_VARIABLE candidate_PAL_path)
                endif()
            endif()
            if(EXISTS ${candidate_PAL_path})
                cmake_path(SET full_name ${candidate_PAL_path})
            endif()
        endif()
    endif()
    cmake_path(ABSOLUTE_PATH full_name BASE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    set(${out_name} ${full_name} PARENT_SCOPE)
endfunction()

function(ly_get_list_relative_pal_filename out_name in_name)
    message(DEPRECATION "ly_get_list_relative_pal_filename is being deprecated, change your code to use o3de_pal_dir instead.")

    # parent relative path is optional
    if(${ARGC} GREATER 4)
        if(ARGV4)
            set(parent_relative_path ${ARGV4})
        endif()
    endif()

    # The Default object path for path is the LY_ROOT_FOLDER
    cmake_path(SET object_path NORMALIZE "${LY_ROOT_FOLDER}")
    if(${ARGC} GREATER 3)
        if(ARGV3)
            # The user has supplied an object restricted path, the object path for consideration
            cmake_path(SET object_path NORMALIZE ${ARGV3})
        endif()
    endif()

    # The default restricted object path is O3DE_ENGINE_RESTRICTED_PATH
    cmake_path(SET object_restricted_path NORMALIZE "${O3DE_ENGINE_RESTRICTED_PATH}")
    if(${ARGC} GREATER 2)
        if(ARGV2)
            # The user has supplied an object restricted path
            cmake_path(SET object_restricted_path NORMALIZE ${ARGV2})
        endif()
    endif()

    if(${ARGC} GREATER 4)
        o3de_pal_dir(abs_name ${in_name} "${object_restricted_path}" "${object_path}" "${parent_relative_path}")
    else()
        o3de_pal_dir(abs_name ${in_name} "${object_restricted_path}" "${object_path}")
    endif()

    cmake_path(RELATIVE_PATH abs_name BASE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE relative_name)
    set(${out_name} ${relative_name} PARENT_SCOPE)
endfunction()

o3de_pal_dir(pal_cmake_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Platform/${PAL_PLATFORM_NAME} "${O3DE_ENGINE_RESTRICTED_PATH}" "${LY_ROOT_FOLDER}")

ly_include_cmake_file_list(${pal_cmake_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake)

include(${pal_cmake_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
include(${pal_cmake_dir}/Toolchain_${PAL_PLATFORM_NAME_LOWERCASE}.cmake OPTIONAL)

set(LY_DISABLE_TEST_MODULES FALSE CACHE BOOL "Option to forcibly disable the inclusion of test targets in the build")

if(LY_DISABLE_TEST_MODULES)
    ly_set(PAL_TRAIT_BUILD_TESTS_SUPPORTED FALSE)
endif()
