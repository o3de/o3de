#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PROJECT_NAME)
    cmake_minimum_required(VERSION 3.22)

    # Utility function to look for an optional 'engine_finder_cmake' path 
    function(get_engine_finder_cmake_path project_json_file_path path_value)
        if(NOT $path_value AND EXISTS "${project_json_file_path}")
            file(READ "${project_json_file_path}" project_json_data)
            string(JSON engine_finder_cmake_value ERROR_VARIABLE json_error GET ${project_json_data} "engine_finder_cmake")
            if(NOT json_error AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${engine_finder_cmake_value}")
                set(path_value "${engine_finder_path}" PARENT_SCOPE)
            endif()
        endif()
    endfunction()
    
    # Check for optional 'engine_finder_cmake' in order of preference
    # We support per-project customization to make it easier to upgrade 
    # or revert to a custom EngineFinder.cmake 
    get_engine_finder_cmake_path("${CMAKE_CURRENT_SOURCE_DIR}/user/project.json" engine_finder_cmake_path)
    get_engine_finder_cmake_path("${CMAKE_CURRENT_SOURCE_DIR}/project.json" engine_finder_cmake_path)
    if(NOT engine_finder_cmake_path)
        set(engine_finder_cmake_path cmake/EngineFinder.cmake)
    endif()

    include(${engine_finder_cmake_path} OPTIONAL)
    find_package(o3de REQUIRED)
    project(AutomatedTesting
        LANGUAGES C CXX
        VERSION 1.0.0.0
    )
    o3de_initialize()
endif()