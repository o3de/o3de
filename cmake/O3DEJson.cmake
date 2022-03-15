#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include_guard()

#! o3de_read_json_external_subdirs
#  Read the "external_subdirectories" array from a *.json file
#  External subdirectories are any folders with CMakeLists.txt in them
#  This could be regular subdirectories, Gems(contains an additional gem.json),
#  Restricted folders(contains an additional restricted.json), etc...
#  
#  \arg:output_external_subdirs name of output variable to store external subdirectories into
#  \arg:input_json_path path to the *.json file to load and read the external subdirectories from
#  \return: external subdirectories as is from the json file.
function(o3de_read_json_external_subdirs output_external_subdirs input_json_path)
    o3de_read_json_array(json_array ${input_json_path} "external_subdirectories")
    set(${output_external_subdirs} ${json_array} PARENT_SCOPE)
endfunction()

#! read_json_array
#  Reads the a json array field into a cmake list variable
function(o3de_read_json_array read_output_array input_json_path array_key)
    ly_file_read(${input_json_path} manifest_json_data)
    string(JSON array_count ERROR_VARIABLE manifest_json_error
        LENGTH ${manifest_json_data} ${array_key})
    if(manifest_json_error)
        # There is no key, return
        return()
    endif()

    if(array_count GREATER 0)
        math(EXPR array_range "${array_count}-1")
        foreach(array_index RANGE ${array_range})
            string(JSON array_element ERROR_VARIABLE manifest_json_error
                GET ${manifest_json_data} ${array_key} "${array_index}")
            if(manifest_json_error)
                message(FATAL_ERROR "Error reading field at index ${array_index} in \"${array_key}\" JSON array: ${manifest_json_error}")
            endif()
            list(APPEND array_elements ${array_element})
        endforeach()
    endif()
    set(${read_output_array} ${array_elements} PARENT_SCOPE)
endfunction()

function(o3de_read_json_key output_value input_json_path key)
    ly_file_read(${input_json_path} manifest_json_data)
    string(JSON value ERROR_VARIABLE manifest_json_error GET ${manifest_json_data} ${key})
    if(manifest_json_error)
        message(FATAL_ERROR "Error reading field at key ${key} in file \"${input_json_path}\" : ${manifest_json_error}")
    endif()
    set(${output_value} ${value} PARENT_SCOPE)
endfunction()
