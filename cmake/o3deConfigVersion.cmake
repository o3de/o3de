#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# This file is included by find_package(o3de CONFIG) and will set PACKAGE_VERSION_COMPATIBLE
# to TRUE or FALSE based on whether the project is compatible with this engine.
# This file also sets PACKAGE_VERSION and PACKAGE_VERSION_EXACT if it can determine
# that information from the engine.json, project.json and <project>/user/project.json.

set(PACKAGE_VERSION_COMPATIBLE FALSE)
set(PACKAGE_VERSION_EXACT FALSE)

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
            elseif(json_error AND ${user_project_engine} STREQUAL "NOTFOUND")
                # When the value is just NOTFOUND that means there is a JSON
                # parsing error, and not simply a missing key 
                message(WARNING "Unable to read 'engine' from '${O3DE_USER_PROJECT_JSON_PATH}'\nError: ${json-error}")
            endif()
        endif()

        set_property(GLOBAL PROPERTY O3DE_PROJECT_JSON ${o3de_project_json})
    else()
        message(WARNING "Unable to read '${O3DE_PROJECT_JSON_PATH}' file necessary to determine whether the project is compatible with this engine.")
        return()
    endif()
endif()


# Get the engine's 'engine_name' and 'version' fields
cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH this_engine_path)
file(READ ${this_engine_path}/engine.json engine_json)
string(JSON engine_name ERROR_VARIABLE json_error GET ${engine_json} engine_name)
if(json_error OR NOT engine_name)
    message(WARNING "Unable to read key 'engine_name' from '${this_engine_path}/engine.json'\nError: ${json_error}")
    return()
endif()
string(JSON engine_version ERROR_VARIABLE json_error GET ${engine_json} version)
if(json_error)
    message(WARNING "Unable to verify engine compatibility because we could not read key 'version' from '${this_engine_path}/engine.json'\nError: ${json_error}")
    return()
endif()

set(PACKAGE_VERSION ${engine_version})

# Get the project.json 'engine' field
string(JSON project_engine ERROR_VARIABLE json_error GET ${o3de_project_json} engine)
if(json_error OR NOT project_engine)
    if(NOT project_engine)
        # Check if the project folder is a subdirectory of this engine and would
        # be found using scan up logic 
        cmake_path(IS_PREFIX this_engine_path "${CMAKE_CURRENT_SOURCE_DIR}" NORMALIZE is_in_engine_tree)
        if(is_in_engine_tree)
            message(VERBOSE "The project folder is a subdirectory of this engine.")
            set(PACKAGE_VERSION_COMPATIBLE TRUE)
            return()
        endif()
    endif()

    message(WARNING "Unable to read 'engine' value from '${O3DE_PROJECT_JSON_PATH}'. Please verify this project is registered with an engine. \nError: ${json_error}")
    return()
endif()

# Split the engine field into engine name and version specifier 
unset(project_engine_name)
unset(project_engine_op)
unset(project_engine_version)
if("${project_engine}" MATCHES "^(.*)(~=|==|!=|<=|>=|<|>|===)(.*)$")
    if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 1)
        set(project_engine_name ${CMAKE_MATCH_1})
    endif()
    if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 2)
        set(project_engine_op ${CMAKE_MATCH_2})
    endif()
    if(${CMAKE_MATCH_COUNT} GREATER_EQUAL 3)
        set(project_engine_version ${CMAKE_MATCH_3})
    endif()
else()
    # format unknown, assume it's just the dependency name
    set(project_engine_name ${project_engine})
endif()

# Does the engine name match?
if(NOT engine_name STREQUAL project_engine_name)
    message(VERBOSE " Engine name '${engine_name}' found in '${this_engine_path}/engine.json' does not match expected engine name '${project_engine_name}'")
    return()
endif()

# Is the version compatible?
if(project_engine_op AND project_engine_version)
    set(contains_version FALSE)
    set(op ${project_engine_op})
    set(specifier_version ${project_engine_version})
    set(version ${engine_version})

    if(op STREQUAL "==" AND version VERSION_EQUAL specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL "!=" AND NOT version VERSION_EQUAL specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL "<=" AND version VERSION_LESS_EQUAL specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL ">=" AND version VERSION_GREATER_EQUAL specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL "<" AND version VERSION_LESS specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL ">" AND version VERSION_GREATER specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL "===" AND version STREQUAL specifier_version)
        set(contains_version TRUE)
    elseif(op STREQUAL "~=")
        # compatible versions have an equivalent combination of >= and == 
        # e.g. ~=2.2 is equivalent to >=2.2,==2.*
        if(version VERSION_GREATER_EQUAL specifier_version)
            string(REPLACE "." ";" specifer_version_part_list ${specifier_version})
            list(LENGTH specifer_version_part_list list_length)
            if(list_length LESS 2)
                # truncating would leave nothing to compare 
                set(contains_version TRUE)
            else()
                # trim the last version part because CMake doesn't support '*'
                math(EXPR truncated_length "${list_length} - 1")
                list(SUBLIST specifer_version_part_list 0 ${truncated_length} specifier_version)
                string(REPLACE ";" "." specifier_version "${specifier_version}")
                string(REPLACE "." ";" version_part_list ${version})
                list(SUBLIST version_part_list 0 ${truncated_length} version)
                string(REPLACE ";" "." version "${version}")

                # compare the truncated versions
                if(version VERSION_EQUAL specifier_version)
                    set(contains_version TRUE)
                endif()
            endif()
        endif()
    endif()

    if(NOT contains_version)
        message(VERBOSE "The engine ${engine_name} version ${engine_version} at ${this_engine_path} is not compatible with the project's version specifier '${project_engine_op}${project_engine_version}'")
        return()
    endif()

    message(VERBOSE "The engine '${engine_name}' version '${engine_version}' at '${this_engine_path}' is compatible with the project's version specifier '${project_engine_op}${project_engine_version}'")
else()
    message(VERBOSE "The engine '${engine_name}' version '${engine_version}' at '${this_engine_path}' is compatible because the project has no engine version specifier.'")
endif()

set(PACKAGE_VERSION_COMPATIBLE TRUE)

if(project_engine_version)
    if(PACKAGE_VERSION STREQUAL ${project_engine_version})
        set(PACKAGE_VERSION_EXACT TRUE)
    endif()
endif()
