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
file(READ ${CMAKE_CURRENT_LIST_DIR}/project.json project_json)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/project.json)

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

if(DEFINED ENV{USERPROFILE} AND EXISTS $ENV{USERPROFILE})
    set(manifest_path $ENV{USERPROFILE}/.o3de/o3de_manifest.json) # Windows
else()
    set(manifest_path $ENV{HOME}/.o3de/o3de_manifest.json) # Unix
endif()

set(registration_error [=[
Engine registration is required before configuring a project.
Run 'scripts/o3de register --this-engine' from the engine root.
]=])

# Read the ~/.o3de/o3de_manifest.json file and look through the 'engines_path' object.
# Find a key that matches LY_ENGINE_NAME_TO_USE and use that as the engine path.
if(EXISTS ${manifest_path})
    file(READ ${manifest_path} manifest_json)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${manifest_path})

    string(JSON engines_path_count ERROR_VARIABLE json_error LENGTH ${manifest_json} engines_path)
    if(json_error)
        message(FATAL_ERROR "Unable to read key 'engines_path' from '${manifest_path}'\nError: ${json_error}\n${registration_error}")
    endif()

    string(JSON engines_path_type ERROR_VARIABLE json_error TYPE ${manifest_json} engines_path)
    if(json_error OR NOT ${engines_path_type} STREQUAL "OBJECT")
        message(FATAL_ERROR "Type of 'engines_path' in '${manifest_path}' is not a JSON Object\nError: ${json_error}")
    endif()

    math(EXPR engines_path_count "${engines_path_count}-1")
    foreach(engine_path_index RANGE ${engines_path_count})
        string(JSON engine_name ERROR_VARIABLE json_error MEMBER ${manifest_json} engines_path ${engine_path_index})
        if(json_error)
            message(FATAL_ERROR "Unable to read 'engines_path/${engine_path_index}' from '${manifest_path}'\nError: ${json_error}")
        endif()

        if(LY_ENGINE_NAME_TO_USE STREQUAL engine_name)
            string(JSON engine_path ERROR_VARIABLE json_error GET ${manifest_json} engines_path ${engine_name})
            if(json_error)
                message(FATAL_ERROR "Unable to read value from 'engines_path/${engine_name}'\nError: ${json_error}")
            endif()

            if(engine_path)
                list(APPEND CMAKE_MODULE_PATH "${engine_path}/cmake")
                return()
            endif()
        endif()
    endforeach()
    
    message(FATAL_ERROR "The project.json uses engine name '${LY_ENGINE_NAME_TO_USE}' but no engine with that name has been registered.\n${registration_error}")
else()
    # If the user is passing CMAKE_MODULE_PATH we assume thats where we will find the engine
    if(NOT CMAKE_MODULE_PATH)
        message(FATAL_ERROR "O3DE Manifest file not found.\n${registration_error}")
    endif()
endif()
