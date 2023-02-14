#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This file is temporarly a tweaked version of the o3de's root CMakeLists.txt
# Once we have the install step, this file will be generated and wont require adding subdirectories

include(FindPackageHandleStandardArgs)

function(o3de_current_file_path path)
    set(${path} ${CMAKE_CURRENT_FUNCTION_LIST_DIR} PARENT_SCOPE)
endfunction()

o3de_current_file_path(current_path)

# Make sure the cmake configure dependency added here is a normalized path to engine.json,
# because later it's read again using a path like ${LY_ROOT_FOLDER}/engine.json, which
# is also normalized.  They should match to avoid errors on some build systems.
cmake_path(SET engine_json_path NORMALIZE ${current_path}/../engine.json)
file(READ ${engine_json_path} engine_json)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${engine_json_path})

string(JSON this_engine_name ERROR_VARIABLE json_error GET ${engine_json} engine_name)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'engine_name' from '${engine_json_path}', error: ${json_error}")
endif()


# Make sure we are matching O3DE_ENGINE_NAME_TO_USE with the current engine
set(found_matching_engine FALSE)

if(NOT O3DE_ENGINE_NAME_TO_USE)
    # Look for a local engine override in <project>/user/project.json
    cmake_path(SET O3DE_USER_PROJECT_JSON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/user/project.json)
    if(EXISTS "${O3DE_USER_PROJECT_JSON_PATH}")
        file(READ "${O3DE_USER_PROJECT_JSON_PATH}" user_project_json)
        string(JSON O3DE_ENGINE_NAME_TO_USE ERROR_VARIABLE json_error GET ${user_project_json} engine)
    endif()

    # Look in project.json if no local override was found
    if(NOT O3DE_ENGINE_NAME_TO_USE)
        cmake_path(SET O3DE_PROJECT_JSON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/project.json)
        file(READ "${O3DE_PROJECT_JSON_PATH}" project_json)
        string(JSON O3DE_ENGINE_NAME_TO_USE ERROR_VARIABLE json_error GET ${project_json} engine)
        if(json_error)
            message(FATAL_ERROR "Unable to read key 'engine' from '${O3DE_PROJECT_JSON_PATH}'\nPlease register the project with an engine.\nError: ${json_error}")
        endif()
    endif()
endif()

# Support older CMake prefixes
if(LY_ENGINE_NAME_TO_USE AND NOT O3DE_ENGINE_NAME_TO_USE)
    set(O3DE_ENGINE_NAME_TO_USE ${LY_ENGINE_NAME_TO_USE})
endif()

if(this_engine_name STREQUAL O3DE_ENGINE_NAME_TO_USE)
    set(found_matching_engine TRUE)
else()
    message(VERBOSE "Project engine name '${O3DE_ENGINE_NAME_TO_USE}' does not match this engine name '${this_engine_name}' in '${engine_json_path}'")
endif()


find_package_handle_standard_args(o3de
    "The engine name for this engine '${this_engine_name}' does not match the projects engine name '${O3DE_ENGINE_NAME_TO_USE}'."
    found_matching_engine
)

cmake_path(SET engine_root_folder NORMALIZE ${current_path}/..)
set_property(GLOBAL PROPERTY O3DE_ENGINE_ROOT_FOLDER "${engine_root_folder}")

# Inject the CompilerSettings.cmake to be included before the project command
set(CMAKE_PROJECT_INCLUDE_BEFORE "${engine_root_folder}cmake/CompilerSettings.cmake")

macro(o3de_initialize)
    set(INSTALLED_ENGINE FALSE)
    set(LY_PROJECTS ${CMAKE_CURRENT_LIST_DIR})
    o3de_current_file_path(current_path)
    enable_testing()
    add_subdirectory(${current_path}/.. o3de)
endmacro()
