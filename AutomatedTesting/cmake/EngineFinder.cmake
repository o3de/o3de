# {BEGIN_LICENSE}
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# {END_LICENSE}
# Edits to this file may be lost in upgrades. Instead of changing this file, use 
# the 'engine_finder_cmake_path' key in your project.json or user/project.json to specify 
# an alternate .cmake file to use instead of this one.

include_guard()

include(cmake/VersionUtils.cmake)

# Read the engine name from the shared or user project.json
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/project.json project_json)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/project.json)

string(JSON O3DE_ENGINE_NAME_TO_USE ERROR_VARIABLE json_error GET ${project_json} engine)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'engine' from 'project.json'\nError: ${json_error}")
endif()
 
# Let the user override the engine to use
cmake_path(SET user_project_json_path ${CMAKE_CURRENT_SOURCE_DIR}/user/project.json)
if(EXISTS "${user_project_json_path}")
    file(READ "${user_project_json_path}" user_project_json)
    string(JSON user_project_engine_name ERROR_VARIABLE json_error GET ${user_project_json} engine)
    if(NOT json_error)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${user_project_json_path}")
        set(O3DE_ENGINE_NAME_TO_USE ${user_project_engine_name})
    endif()
    string(JSON user_engine_path ERROR_VARIABLE json_error GET ${user_project_json} "engine_path")
endif()

# Extract optional version specifier in the engine name 
o3de_get_version_specifier_parts("${O3DE_ENGINE_NAME_TO_USE}" O3DE_ENGINE_NAME_TO_USE O3DE_ENGINE_SPECIFIER_OP O3DE_ENGINE_SPECIFIER_VERSION)

# for backward compatibility
set(LY_ENGINE_NAME_TO_USE ${O3DE_ENGINE_NAME_TO_USE})

if(CMAKE_MODULE_PATH)
    foreach(module_path ${CMAKE_MODULE_PATH})
        if(EXISTS "${module_path}/Findo3de.cmake")
            cmake_path(GET module_path PARENT_PATH candidate_engine_path)
            o3de_engine_compatible("${candidate_engine_path}" ${O3DE_ENGINE_NAME_TO_USE} ${O3DE_ENGINE_SPECIFIER_OP} ${O3DE_ENGINE_SPECIFIER_VERSION} is_compatible engine_version FALSE)
            if(is_compatible)
                return() # Engine being forced through CMAKE_MODULE_PATH
            endif()
        endif()
    endforeach()
endif()

# Read the user/project.json file and look for the engine_path
if(user_engine_path)
    if(EXISTS ${user_engine_path}/cmake)
        # always be verbose on this compatibility check
        o3de_engine_compatible(${user_engine_path} ${O3DE_ENGINE_NAME_TO_USE} ${O3DE_ENGINE_SPECIFIER_OP} ${O3DE_ENGINE_SPECIFIER_VERSION} is_compatible engine_version TRUE)
        if(is_compatible)
            message(STATUS "Selecting engine from '${user_project_json_path}' at '${user_engine_path}'")
            list(APPEND CMAKE_MODULE_PATH "${user_engine_path}/cmake")
            return()
        endif()
    else()
        string(CONCAT engine_path_info "Unable to use the engine_path from "
            "'${user_project_json_path}' because "
            "'${user_engine_path}/cmake' does not exist.\n"
            "Falling back to look for an engine named '${O3DE_ENGINE_NAME_TO_USE}' in o3de_manifest.json")
        message(INFO " ${engine_path_info}")
    endif()
endif()

if(DEFINED ENV{USERPROFILE} AND EXISTS $ENV{USERPROFILE})
    set(manifest_path $ENV{USERPROFILE}/.o3de/o3de_manifest.json) # Windows
else()
    set(manifest_path $ENV{HOME}/.o3de/o3de_manifest.json) # Unix
endif()

set(registration_error [=[
Engine registration is required before configuring a project.
Run 'scripts/o3de register --this-engine' from the engine root.
]=])

function(o3de_get_most_compatible_engine manifest_json output_engine_path output_engine_version)
    string(JSON engines_count ERROR_VARIABLE json_error LENGTH ${manifest_json} engines)
    if(json_error)
        message(FATAL_ERROR "Unable to read key 'engines' from '${manifest_path}'\nError: ${json_error}\n${registration_error}")
    endif()

    string(JSON engines_type ERROR_VARIABLE json_error TYPE ${manifest_json} engines)
    if(json_error OR NOT ${engines_type} STREQUAL "ARRAY")
        message(FATAL_ERROR "Type of 'engines' in '${manifest_path}' is not a JSON ARRAY\nError: ${json_error}")
    endif()

    math(EXPR engines_count "${engines_count}-1")

    set(most_compatible_engine_path "")
    set(most_compatible_engine_version "")

    foreach(array_index RANGE ${engines_count})
        string(JSON engine_path ERROR_VARIABLE json_error GET ${manifest_json} engines "${array_index}")
        if(json_error)
            message(FATAL_ERROR "Unable to read 'engines/${array_index}' from '${manifest_path}'\nError: ${json_error}")
        endif()

        if(EXISTS ${engine_path}/engine.json)
            o3de_engine_compatible(${engine_path} ${O3DE_ENGINE_NAME_TO_USE} ${O3DE_ENGINE_SPECIFIER_OP} ${O3DE_ENGINE_SPECIFIER_VERSION} is_compatible engine_version FALSE)
            if(is_compatible)
                if(NOT most_compatible_engine_path)
                    set(most_compatible_engine_path ${engine_path})
                    set(most_compatible_engine_version ${engine_version})
                    message(VERBOSE "Considering engine '${O3DE_ENGINE_NAME_TO_USE}' version '${engine_version}' from '${engine_path}'")
                elseif(${engine_version} VERSION_GREATER ${most_compatible_engine_version})
                    message(VERBOSE "Considering engine '${O3DE_ENGINE_NAME_TO_USE}' version '${engine_version}' from '${engine_path}' because it has a greater version number")
                    set(most_compatible_engine_path ${engine_path})
                    set(most_compatible_engine_version ${engine_version})
                else()
                    message(VERBOSE "Not considering engine '${O3DE_ENGINE_NAME_TO_USE}' version '${engine_version}' from '${engine_path}' because it does not have a greater version number than the engine at '${most_compatible_engine_path}' (${most_compatible_engine_version})")
                endif()
            endif()
        else()
            message(WARNING "${engine_path}/engine.json not found")
        endif()
    endforeach()

    set(${output_engine_path} ${most_compatible_engine_path} PARENT_SCOPE)
    set(${output_engine_version} ${most_compatible_engine_version} PARENT_SCOPE)
endfunction()


# Read the ~/.o3de/o3de_manifest.json file and look through the 'engines' array.
# Search all known engines for one with the matching name and compatible version
if(EXISTS ${manifest_path})
    file(READ ${manifest_path} manifest_json)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${manifest_path})

    o3de_get_most_compatible_engine(${manifest_json} most_compatible_engine_path most_compatible_engine_version )

    if(most_compatible_engine_path)
        message(STATUS "Selecting most compatible engine '${O3DE_ENGINE_NAME_TO_USE}' version '${most_compatible_engine_version}' found at '${most_compatible_engine_path}'")
        list(APPEND CMAKE_MODULE_PATH "${most_compatible_engine_path}/cmake")
        return()
    endif()

    if(O3DE_ENGINE_SPECIFIER_OP AND O3DE_ENGINE_SPECIFIER_VERSION)
        message(FATAL_ERROR "The project.json uses engine '${O3DE_ENGINE_NAME_TO_USE}${O3DE_ENGINE_SPECIFIER_OP}${O3DE_ENGINE_SPECIFIER_VERSION}' but no engine with that name and compatible version has been registered.\n${registration_error}")
    else()
        message(FATAL_ERROR "The project.json uses engine name '${O3DE_ENGINE_NAME_TO_USE}' but no engine with that name has been registered.\n${registration_error}")
    endif()
else()
    # If the user is passing CMAKE_MODULE_PATH we assume thats where we will find the engine
    if(NOT CMAKE_MODULE_PATH)
        message(FATAL_ERROR "O3DE Manifest file not found at '${manifest_path}'.\n${registration_error}")
    endif()
endif()
