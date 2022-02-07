#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PROJECT_NAME)
    cmake_minimum_required(VERSION 3.20)
    include(cmake/CompilerSettings.cmake)
    project(AutomatedTesting
        LANGUAGES C CXX
        VERSION 1.0.0.0
    )
    include(cmake/EngineFinder.cmake OPTIONAL)
    find_package(o3de REQUIRED)
    o3de_initialize()
else()
    # Add the project_name to global LY_PROJECTS_TARGET_NAME property
    file(READ "${CMAKE_CURRENT_LIST_DIR}/project.json" project_json)

    string(JSON project_target_name ERROR_VARIABLE json_error GET ${project_json} "project_name")
    if(json_error)
        message(FATAL_ERROR "Unable to read key 'project_name' from 'project.json'")
    endif()

    set_property(GLOBAL APPEND PROPERTY LY_PROJECTS_TARGET_NAME ${project_target_name})

    add_subdirectory(Gem)
endif()