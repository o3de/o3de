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


# Use CMAKE_CURRENT_FUNCTION_LIST_DIR in case an older projects used add_directory()
function(o3de_current_file_path path)
    set(${path} ${CMAKE_CURRENT_FUNCTION_LIST_DIR} PARENT_SCOPE)
endfunction()

o3de_current_file_path(current_path)

# Make sure the cmake configure dependency added here is a normalized path to engine.json,
# because later it's read again using a path like ${LY_ROOT_FOLDER}/engine.json, which
# is also normalized.  They should match to avoid errors on some build systems.
cmake_path(SET engine_json_path NORMALIZE ${current_path}/../engine.json)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${engine_json_path})

if(NOT LY_ENGINE_NAME_TO_USE)
    # PACKAGE_VERSION_COMPATIBLE should be set TRUE by o3deConfigVersion.cmake
    find_package_handle_standard_args(o3de
        "This engine was not found to be compatible with your project, or no compatibility checks were done. For more information, run the command again with '--log-level VERBOSE'."
        PACKAGE_VERSION_COMPATIBLE
    )
else()
    # LY_ENGINE_NAME_TO_USE compatibility check for older projects 
    set(found_matching_engine FALSE)

    file(READ ${engine_json_path} engine_json)
    string(JSON this_engine_name ERROR_VARIABLE json_error GET ${engine_json} engine_name)
    if(json_error)
        message(FATAL_ERROR "Unable to read key 'engine_name' from '${engine_json_path}', error: ${json_error}")
    endif()

    if(this_engine_name STREQUAL LY_ENGINE_NAME_TO_USE)
        set(found_matching_engine TRUE)
    else()
        message(VERBOSE "Project engine name '${LY_ENGINE_NAME_TO_USE}' does not match this engine name '${this_engine_name}' in '${engine_json_path}'")
    endif()

    find_package_handle_standard_args(o3de
        "The engine name for this engine '${this_engine_name}' does not match the projects engine name '${LY_ENGINE_NAME_TO_USE}'"
        found_matching_engine
    )
endif()

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
