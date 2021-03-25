# {BEGIN_LICENSE}
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
# {END_LICENSE}

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
if($ENV{USERPROFILE} AND EXISTS $ENV{USERPROFILE})
    set(manifest_path $ENV{USERPROFILE}/.o3de/o3de_manifest.json) # Windows
else()
    set(manifest_path $ENV{HOME}/.o3de/o3de_manifest.json) # Unix
endif()

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





