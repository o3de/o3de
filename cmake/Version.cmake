#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(cmake/FileUtil.cmake)

string(TIMESTAMP current_year "%Y")
set(O3DE_COPYRIGHT_YEAR ${current_year} CACHE STRING "Open 3D Engine's copyright year")

get_filename_component(ENGINE_JSON_PATH "${CMAKE_CURRENT_LIST_DIR}/../engine.json" ABSOLUTE)
ly_file_read(${ENGINE_JSON_PATH} manifest_json_data)

#! o3de_read_engine_default: Attempts to read a field from engine.json 
#     that might be overridden by an environment variable. Priority order:
#      1. environment variable
#      2. engine.json value
#      3. default value
#  
# \arg:output_value - name of output variable to set 
# \arg:key - name of field in engine.json 
# \arg:env_value - name of environment variable to use as override 
# \arg:default_value - value to use if neither environment var or engine.json value found 
function(o3de_read_engine_default output_value env_value key default_value description)
    # use Environment override if provided
    if(NOT ${env_value} STREQUAL "")
        set(${output_value} ${env_value} CACHE STRING ${description})
        return()
    endif()

    # pull the value from engine.json if available
    string(JSON value ERROR_VARIABLE manifest_json_error GET ${manifest_json_data} ${key})
    if(manifest_json_error)
        message(WARNING "Failed to read ${key} from \"${ENGINE_JSON_PATH}\" : ${manifest_json_error}")
        set(${value} ${default_value})
    endif()
    set(${output_value} ${value} CACHE STRING ${description})
endfunction()

#! o3de_set_major_minor_patch_with_prefix: Parses a SemVer string and sets
#   separate major, minor and patch variables.  e.g. given "1.2.3" and prefix VER,
#   the following variables are set
#   VER_MAJOR 1
#   VER_MINOR 2
#   VER_PATCH 3
#
# \arg:prefix - prefix for cmake variable names 
# \arg:version_string - input string in SemVer format e.g. 1.2.3 
function(o3de_set_major_minor_patch_with_prefix prefix version_string)
    string(REPLACE "." ";" version_list ${version_string})
    list(GET version_list 0 major)
    list(GET version_list 1 minor)
    list(GET version_list 2 patch)
    set(${prefix}_MAJOR ${major} PARENT_SCOPE)
    set(${prefix}_MINOR ${minor} PARENT_SCOPE)
    set(${prefix}_PATCH ${patch} PARENT_SCOPE)
endfunction()

o3de_read_engine_default(O3DE_VERSION_STRING "$ENV{O3DE_VERSION}" "version" "0.0.0" "Open 3D Engine's version")
o3de_read_engine_default(O3DE_DISPLAY_VERSION_STRING "$ENV{O3DE_DISPLAY_VERSION}" "display_version" "00.00" "Open 3D Engine's display version")
o3de_read_engine_default(O3DE_BUILD_VERSION "$ENV{O3DE_BUILD_VERSION}" "build" 0 "Open 3D Engine's build number")
o3de_read_engine_default(O3DE_ENGINE_NAME "$ENV{O3DE_ENGINE_NAME}" "engine_name" "o3de" "Open 3D Engine's engine name")

o3de_set_major_minor_patch_with_prefix(O3DE_VERSION ${O3DE_VERSION_STRING})
