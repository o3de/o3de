# {BEGIN_LICENSE}
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# {END_LICENSE}
# This file is copied during engine registration. Edits to this file will be lost next
# time a registration happens.

include_guard()

# Read the engine name from the project_json file
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/project.json project_json)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/project.json)

string(JSON LY_ENGINE_NAME_TO_USE ERROR_VARIABLE json_error GET ${project_json} engine)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'engine' from 'project.json'\nError: ${json_error}")
endif()

if(CMAKE_MODULE_PATH)
    foreach(module_path ${CMAKE_MODULE_PATH})
        if(EXISTS ${module_path}/Findo3de.cmake)
            file(READ ${module_path}/../engine.json engine_json)
            string(JSON engine_name ERROR_VARIABLE json_error GET ${engine_json} engine_name)
            if(json_error)
                message(FATAL_ERROR "Unable to read key 'engine_name' from 'engine.json'\nError: ${json_error}")
            endif()
            if(LY_ENGINE_NAME_TO_USE STREQUAL engine_name)
                return() # Engine being forced through CMAKE_MODULE_PATH
            endif()
        endif()
    endforeach()
endif()

# Read the user/project_settings.json file and look for the engine_path
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/user/project_settings.json)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/user/project_settings.json project_settings_json)
    string(JSON engine_path ERROR_VARIABLE json_error GET ${project_settings_json} "engine_path")
    if(json_error)
        string(CONCAT project_registration_error "Unable to read key 'engine_path' from "
            "'${CMAKE_CURRENT_SOURCE_DIR}/user/project_settings.json'\n"
            "Error: ${json_error}\n"
            "Project registration is required before configuring. "
            "Register this project by adding it to an engine in Project Manager or run "
            "the following command from the engine root:\n"
            "scripts/o3de register --pp \"${CMAKE_CURRENT_SOURCE_DIR}\"\n")
        message(FATAL_ERROR "${project_registration_error}")
    elseif(EXISTS ${engine_path}/cmake)
        message(INFO " Using engine from user/project_settings.json ${engine_path}")
        list(APPEND CMAKE_MODULE_PATH "${engine_path}/cmake")
        return()
    else()
        string(CONCAT engine_path_info "Unable to use the engine_path from "
            "'${CMAKE_CURRENT_SOURCE_DIR}/user/project_settings.json' because "
            "'${engine_path}/cmake' does not exist.\n"
            "Falling back to look for an engine named '${LY_ENGINE_NAME_TO_USE}' in o3de_manifest.json")
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

# Read the ~/.o3de/o3de_manifest.json file and look through the 'engines' array.
# Search all known engines for one with the matching name.
if(EXISTS ${manifest_path})
    file(READ ${manifest_path} manifest_json)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${manifest_path})

    string(JSON engines_count ERROR_VARIABLE json_error LENGTH ${manifest_json} engines)
    if(json_error)
        message(FATAL_ERROR "Unable to read key 'engines' from '${manifest_path}'\nError: ${json_error}\n${registration_error}")
    endif()

    string(JSON engines_type ERROR_VARIABLE json_error TYPE ${manifest_json} engines)
    if(json_error OR NOT ${engines_type} STREQUAL "ARRAY")
        message(FATAL_ERROR "Type of 'engines' in '${manifest_path}' is not a JSON ARRAY\nError: ${json_error}")
    endif()

    math(EXPR engines_count "${engines_count}-1")
    foreach(array_index RANGE ${engines_count})
        string(JSON engine_path ERROR_VARIABLE json_error GET ${manifest_json} engines "${array_index}")
        if(json_error)
            message(FATAL_ERROR "Unable to read 'engines/${array_index}' from '${manifest_path}'\nError: ${json_error}")
        endif()

        if(EXISTS ${engine_path}/engine.json)
            file(READ ${engine_path}/engine.json engine_json)
            string(JSON engine_name ERROR_VARIABLE json_error GET ${engine_json} engine_name)
            if(json_error)
                message(FATAL_ERROR "Unable to read key 'engine_name' from '${engine_path}/engine.json'\nError: ${json_error}")
            elseif(LY_ENGINE_NAME_TO_USE STREQUAL engine_name)
                list(APPEND CMAKE_MODULE_PATH "${engine_path}/cmake")

                message(INFO " Found an engine named ${LY_ENGINE_NAME_TO_USE} at ${engine_path}")
                # save this location in user/project_settings.json for faster retrieval
                file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/user/project_settings.json "{
    \"engine_path\":\"${engine_path}\"
}")
                return()
            endif()
        else()
            message(WARNING "${engine_path}/engine.json not found")
        endif()
    endforeach()


    message(FATAL_ERROR "The project.json uses engine name '${LY_ENGINE_NAME_TO_USE}' but no engine with that name has been registered.\n${registration_error}")
else()
    # If the user is passing CMAKE_MODULE_PATH we assume thats where we will find the engine
    if(NOT CMAKE_MODULE_PATH)
        message(FATAL_ERROR "O3DE Manifest file not found.\n${registration_error}")
    endif()
endif()
