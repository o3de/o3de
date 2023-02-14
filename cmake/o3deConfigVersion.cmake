#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# This file is included by find_package(o3de CONFIG) and will set PACKAGE_VERSION_COMPATIBLE
# to TRUE or FALSE based on whether the project is compatible with this engine.
# This file also sets PACKAGE_VERSION and PACKAGE_VERISON_EXACT if it can determine
# that information from the engine.json, project.json and <project>/user/project.json.

set(PACKAGE_VERSION_COMPATIBLE FALSE)
set(PACKAGE_VERSION_EXACT FALSE)

# Helper function needed to resolve current path
function(o3de_current_file_path path)
    set(${path} ${CMAKE_CURRENT_FUNCTION_LIST_DIR} PARENT_SCOPE)
endfunction()
o3de_current_file_path(current_path)

include(${current_path}/VersionUtils.cmake)

# Store the project.json with any overrides from <project>/user/project.json 
# in a global property to avoid the performance hit of loading multiple times
# The project's CMake will likely have already set this for us, but just in case
get_property(o3de_project_json GLOBAL PROPERTY O3DE_PROJECT_JSON)
cmake_path(SET O3DE_PROJECT_JSON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/project.json)
if(NOT o3de_project_json)
    if(EXISTS ${O3DE_PROJECT_JSON_PATH})
        file(READ "${O3DE_PROJECT_JSON_PATH}" o3de_project_json)

        # Allow the user to override the 'engine' value in <project>/user/project.json
        cmake_path(SET O3DE_USER_PROJECT_JSON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/user/project.json)
        if(EXISTS ${O3DE_USER_PROJECT_JSON_PATH})
            file(READ "${O3DE_USER_PROJECT_JSON_PATH}" o3de_user_project_json)
            string(JSON user_project_engine ERROR_VARIABLE json_error GET ${o3de_user_project_json} engine)
            if(user_project_engine AND NOT json_error)
                string(JSON o3de_project_json SET "${o3de_project_json}" engine "\"${user_project_engine}\"" )
            endif()
        endif()

        set_property(GLOBAL PROPERTY O3DE_PROJECT_JSON ${o3de_project_json})
    else()
        message(FATAL_ERROR "Unable to read '${O3DE_PROJECT_JSON_PATH}' file necessary to determine whether the project is compatible with this engine.")
    endif()
endif()

string(JSON project_engine ERROR_VARIABLE json_error GET ${o3de_project_json} engine)
if(project_engine AND NOT json_error)
    # Get the engine name and version specifier information for the project from the 'engine' field
    o3de_get_version_specifier_parts(${project_engine} project_engine_name specifier_op specifier_version)

    cmake_path(GET current_path PARENT_PATH this_engine_path)
    o3de_engine_compatible(${this_engine_path} ${project_engine_name} ${specifier_op} ${specifier_version} is_compatible this_engine_version)
    if(is_compatible)
        set(PACKAGE_VERSION ${this_engine_version})
        set(PACKAGE_VERSION_COMPATIBLE TRUE)
        if(PACKAGE_VERSION STREQUAL ${specifier_version})
            set(PACKAGE_VERSION_EXACT TRUE)
        endif()
    endif()
else()
    message(WARNING "Unable to read 'engine' value from '${O3DE_PROJECT_JSON_PATH}'. Please verify this project is registered with an engine. \nError: ${json_error}")
endif()


