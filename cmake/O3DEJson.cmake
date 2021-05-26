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

include_guard()

set(LY_EXTERNAL_SUBDIRS "" CACHE STRING "List of subdirectories to recurse into when running cmake against the engine's CMakeLists.txt")

#! read_json_external_subdirs
# Read the "external_subdirectories" array from a *.json file
# External subdirectories are any folders with CMakeLists.txt in them
# This could be regular subdirectories, Gems(contains an additional gem.json),
# Restricted folders(contains an additional restricted.json), etc...
#
# \arg:output_external_subdirs name of output variable to store external subdirectories into
# \arg:input_json_path path to the *.json file to load and read the external subdirectories from
# \return: external subdirectories as is from the json file.
function(read_json_external_subdirs output_external_subdirs input_json_path)
    file(READ ${input_json_path} manifest_json_data)
    string(JSON external_subdirs_count ERROR_VARIABLE manifest_json_error
        LENGTH ${manifest_json_data} "external_subdirectories")
    if(manifest_json_error)
        # There is "external_subdirectories" key, so theire are no subdirectories to read
        return()
    endif()

    if(external_subdirs_count GREATER 0)
        math(EXPR external_subdir_range "${external_subdirs_count}-1")
        foreach(external_subdir_index RANGE ${external_subdir_range})
            string(JSON external_subdir ERROR_VARIABLE manifest_json_error
                GET ${manifest_json_data} "external_subdirectories" "${external_subdir_index}")
            if(manifest_json_error)
                message(FATAL_ERROR "Error reading field at index ${external_subdir_index} in \"external_subdirectories\" JSON array: ${manifest_json_error}")
            endif()
            list(APPEND external_subdirs ${external_subdir})
        endforeach()
    endif()
    set(${output_external_subdirs} ${external_subdirs} PARENT_SCOPE)
endfunction()

function(o3de_read_json_key output_value input_json_path key)
    file(READ ${input_json_path} manifest_json_data)
    string(JSON value ERROR_VARIABLE manifest_json_error GET ${manifest_json_data} ${key})
    if(manifest_json_error)
        message(FATAL_ERROR "Error reading field at key ${key} in file \"${input_json_path}\" : ${manifest_json_error}")
    endif()
    set(${output_value} ${value} PARENT_SCOPE)
endfunction()
