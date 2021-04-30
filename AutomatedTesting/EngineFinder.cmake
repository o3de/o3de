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
# This file is copied during engine registration. Edits to this file will be lost next
# time a registration happens.

include_guard()

# Read the engine name from the project_json file
file(READ ${CMAKE_CURRENT_LIST_DIR}/project.json project_json)
string(JSON LY_ENGINE_NAME_TO_USE ERROR_VARIABLE json_error GET ${project_json} engine)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'engine' from 'project.json', error: ${json_error}")
endif()

# Read the list of paths from ~.o3de/o3de_manifest.json
file(TO_CMAKE_PATH "$ENV{USERPROFILE}" home_directory) # Windows
if((NOT home_directory) OR (NOT EXISTS ${home_directory}))
    file(TO_CMAKE_PATH "$ENV{HOME}" home_directory)# Unix
endif()

if (NOT home_directory)
    message(FATAL_ERROR "Cannot find user home directory, the o3de manifest cannot be found")
endif()
# Set manifest path to path in the user home directory
set(manifest_path ${home_directory}/.o3de/o3de_manifest.json)

if(EXISTS ${manifest_path})
    file(READ ${manifest_path} manifest_json)
    string(JSON engines_count ERROR_VARIABLE json_error LENGTH ${manifest_json} engines)
    if(json_error)
        message(FATAL_ERROR "Unable to read key 'engines' from '${manifest_path}', error: ${json_error}")
    endif()

    math(EXPR engines_count "${engines_count}-1")
    foreach(engine_path_index RANGE ${engines_count})
        string(JSON engine_path ERROR_VARIABLE json_error GET ${manifest_json} engines ${engine_path_index})
        if(${json_error})
            message(FATAL_ERROR "Unable to read engines[${engine_path_index}] '${manifest_path}', error: ${json_error}")
        endif()
        list(APPEND CMAKE_MODULE_PATH "${engine_path}/cmake")
    endforeach()
endif()
