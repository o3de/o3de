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

# This file is temporarly a tweaked version of the o3de's root CMakeLists.txt
# Once we have the install step, this file will be generated and wont require adding subdirectories

include(FindPackageHandleStandardArgs)

function(o3de_current_file_path path)
    set(${path} ${CMAKE_CURRENT_FUNCTION_LIST_DIR} PARENT_SCOPE)
endfunction()

o3de_current_file_path(current_path)

# Make sure we are matching LY_ENGINE_NAME_TO_USE with the current engine
file(READ ${current_path}/../engine.json engine_json)
string(JSON this_engine_name ERROR_VARIABLE json_error GET ${engine_json} engine_name)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'engine_name' from '${current_path}/../engine.json', error: ${json_error}")
endif()

if(NOT this_engine_name STREQUAL LY_ENGINE_NAME_TO_USE)
    set(o3de_FOUND FALSE)
    set(o3de_NOT_FOUND_MESSAGE)
    find_package_handle_standard_args(o3de
        "Could not find an engine with matching ${LY_ENGINE_NAME_TO_USE}"
        o3de_FOUND
    )
    return()
endif()

macro(o3de_initialize)
    set(INSTALLED_ENGINE FALSE)
    set(LY_PROJECTS ${CMAKE_CURRENT_LIST_DIR})
    o3de_current_file_path(current_path)
    add_subdirectory(${current_path}/.. o3de)
endmacro()

message(STATUS "Found ${this_engine_name} in ${current_path}")
set(o3de_FOUND FALSE)
find_package_handle_standard_args(o3de
    o3de_FOUND
)