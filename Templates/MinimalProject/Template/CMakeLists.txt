# {BEGIN_LICENSE}
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# {END_LICENSE}

if(NOT PROJECT_NAME)
    cmake_minimum_required(VERSION 3.22)

    # Utility function to look for an optional 'engine_finder_cmake_path'setting 
    function(get_engine_finder_cmake_path project_json_file_path path_value)
        if(NOT ${path_value} AND EXISTS "${project_json_file_path}")
            file(READ "${project_json_file_path}" project_json_data)
            string(JSON engine_finder_cmake_value ERROR_VARIABLE json_error GET ${project_json_data} "engine_finder_cmake_path")
            cmake_path(APPEND CMAKE_CURRENT_SOURCE_DIR "${engine_finder_cmake_value}" engine_finder_cmake_value)
            if(NOT json_error AND EXISTS "${engine_finder_cmake_value}")
                set(${path_value} "${engine_finder_cmake_value}" PARENT_SCOPE)
            elseif(json_error AND ${engine_finder_cmake_value} STREQUAL "NOTFOUND")
                # When the error value is just NOTFOUND that means there is a JSON
                # parsing error, and not simply a missing key 
                message(WARNING "Unable to read 'engine_finder_cmake_path'.\nError: ${json_error} ${engine_finder_cmake_value}")
            endif()
        endif()
    endfunction()
    
    # Check for optional 'engine_finder_cmake_path' in order of preference
    # We support per-project customization to make it easier to upgrade 
    # or revert to a custom EngineFinder.cmake 
    get_engine_finder_cmake_path("${CMAKE_CURRENT_SOURCE_DIR}/user/project.json" engine_finder_cmake_path)
    get_engine_finder_cmake_path("${CMAKE_CURRENT_SOURCE_DIR}/project.json" engine_finder_cmake_path)
    if(NOT engine_finder_cmake_path)
        set(engine_finder_cmake_path cmake/EngineFinder.cmake)
    endif()

    include(${engine_finder_cmake_path} OPTIONAL)
    find_package(o3de REQUIRED)
    project(${Name}
        LANGUAGES C CXX
        VERSION 1.0.0.0
    )
    o3de_initialize()
endif()
