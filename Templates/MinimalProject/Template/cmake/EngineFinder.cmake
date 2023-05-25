# {BEGIN_LICENSE}
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# {END_LICENSE}
# Edits to this file may be lost in upgrades. Instead of changing this file, use 
# the 'engine_finder_cmake_path' key in your project.json or user/project.json to specify 
# an alternate .cmake file to use instead of this one.

include_guard()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/project.json)

# Option 1: Use engine manually set in CMAKE_MODULE_PATH
# CMAKE_MODULE_PATH must contain a path to an engine's cmake folder 
if(CMAKE_MODULE_PATH)
    foreach(module_path ${CMAKE_MODULE_PATH})
        cmake_path(SET module_engine_version_cmake_path "${module_path}/o3deConfigVersion.cmake")
        if(EXISTS "${module_engine_version_cmake_path}")
            include("${module_engine_version_cmake_path}")
            if(PACKAGE_VERSION_COMPATIBLE)
                message(STATUS "Selecting engine from CMAKE_MODULE_PATH '${module_path}'")
                return()
            else()
                message(WARNING "Not using engine from CMAKE_MODULE_PATH '${module_path}' because it is not compatible with this project.")
            endif()
        endif()
    endforeach()
    message(VERBOSE "No compatible engine found from CMAKE_MODULE_PATH '${CMAKE_MODULE_PATH}'.")
endif()

# Option 2: Use the engine from the 'engine_path' field in <project>/user/project.json
cmake_path(SET O3DE_USER_PROJECT_JSON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/user/project.json)
if(EXISTS "${O3DE_USER_PROJECT_JSON_PATH}")
    file(READ "${O3DE_USER_PROJECT_JSON_PATH}" user_project_json)
    if(user_project_json)
        string(JSON user_project_engine_path ERROR_VARIABLE json_error GET ${user_project_json} engine_path)
        if(user_project_engine_path AND NOT json_error)
            cmake_path(SET user_engine_version_cmake_path "${user_project_engine_path}/cmake/o3deConfigVersion.cmake")
            if(EXISTS "${user_engine_version_cmake_path}")
                include("${user_engine_version_cmake_path}")
                if(PACKAGE_VERSION_COMPATIBLE)
                    message(STATUS "Selecting engine '${user_project_engine_path}' from 'engine_path' in '<project>/user/project.json'.")
                    list(APPEND CMAKE_MODULE_PATH "${user_project_engine_path}/cmake")
                    return()
                else()
                    message(FATAL_ERROR "The engine at '${user_project_engine_path}' from 'engine_path' in '${O3DE_USER_PROJECT_JSON_PATH}' is not compatible with this project. Please register this project with a compatible engine, or remove the local override by running:\nscripts\\o3de edit-project-properties -pp ${CMAKE_CURRENT_SOURCE_DIR} --user --engine-path \"\"")
                endif()
            else()
                message(FATAL_ERROR "This project cannot use the engine at '${user_project_engine_path}' because the version cmake file '${user_engine_version_cmake_path}' needed to check compatibility is missing.  \nPlease register this project with a compatible engine, or remove the local override by running:\nscripts\\o3de edit-project-properties -pp ${CMAKE_CURRENT_SOURCE_DIR} --user --engine-path \"\"\nIf you want this project to use an older engine(not recommended), provide a custom EngineFinder.cmake using the o3de CLI's --engine-finder-cmake-path option. ")
            endif()
        elseif(json_error AND ${user_project_engine_path} STREQUAL "NOTFOUND")
            # When the value is just NOTFOUND that means there is a JSON
            # parsing error, and not simply a missing key 
            message(FATAL_ERROR "Unable to read 'engine_path' from '${user_project_engine_path}'\nError: ${json-error}")
        endif()
    endif()
endif()


# Option 3: Find a compatible engine registered in ~/.o3de/o3de_manifest.json 
if(DEFINED ENV{USERPROFILE} AND EXISTS $ENV{USERPROFILE})
    set(manifest_path $ENV{USERPROFILE}/.o3de/o3de_manifest.json) # Windows
else()
    set(manifest_path $ENV{HOME}/.o3de/o3de_manifest.json) # Unix
endif()

set(registration_error [=[
To enable more verbose logging, run the cmake command again with '--log-level VERBOSE'
Engine registration is required before configuring a project.
Run 'scripts/o3de register --this-engine' from the engine root.
]=])

# Create a list of all engines
if(EXISTS ${manifest_path})
    file(READ ${manifest_path} manifest_json)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${manifest_path})

    string(JSON engines_count ERROR_VARIABLE json_error LENGTH ${manifest_json} engines)
    if(json_error)
        message(FATAL_ERROR "Unable to read key 'engines' from '${manifest_path}'\nError: ${json_error}\n${registration_error}")
    endif()

    string(JSON engines_type ERROR_VARIABLE json_error TYPE ${manifest_json} engines)
    if(json_error OR NOT ${engines_type} STREQUAL "ARRAY")
        message(FATAL_ERROR "Type of 'engines' in '${manifest_path}' is not a JSON ARRAY\nError: ${json_error}\n${registration_error}")
    endif()

    math(EXPR engines_count "${engines_count}-1")
    foreach(array_index RANGE ${engines_count})
        string(JSON manifest_engine_path ERROR_VARIABLE json_error GET ${manifest_json} engines "${array_index}")
        if(json_error)
            message(FATAL_ERROR "Unable to read 'engines/${array_index}' from '${manifest_path}'\nError: ${json_error}\n${registration_error}")
        endif()
        list(APPEND O3DE_ENGINE_PATHS ${manifest_engine_path})
    endforeach()
elseif(NOT CMAKE_MODULE_PATH)
    message(FATAL_ERROR "O3DE Manifest file not found at '${manifest_path}'.\n${registration_error}")
endif()

# We cannot just run find_package() on the list of engine paths because
# CMAKE_FIND_PACKAGE_SORT_ORDER sorts based on file name and chooses
# the first package that returns PACKAGE_VERSION_COMPATIBLE 
set(O3DE_MOST_COMPATIBLE_ENGINE_PATH "")
set(O3DE_MOST_COMPATIBLE_ENGINE_VERSION "")
foreach(manifest_engine_path IN LISTS O3DE_ENGINE_PATHS) 
    # Does this engine have a config version cmake file?
    cmake_path(SET version_cmake_path "${manifest_engine_path}/cmake/o3deConfigVersion.cmake")
    if(NOT EXISTS "${version_cmake_path}") 
        message(VERBOSE "Ignoring '${manifest_engine_path}' because no config version cmake file was found at '${version_cmake_path}'")
        continue()
    endif()

    unset(PACKAGE_VERSION)
    unset(PACKAGE_VERSION_COMPATIBLE)
    include("${version_cmake_path}")

    # Follow the version checking convention from find_package(CONFIG)
    if(PACKAGE_VERSION_COMPATIBLE)
        if(NOT O3DE_MOST_COMPATIBLE_ENGINE_PATH) 
            set(O3DE_MOST_COMPATIBLE_ENGINE_PATH "${manifest_engine_path}") 
            set(O3DE_MOST_COMPATIBLE_ENGINE_VERSION ${PACKAGE_VERSION}) 
            message(VERBOSE "Found compatible engine '${manifest_engine_path}' with version '${PACKAGE_VERSION}'")
        elseif(${PACKAGE_VERSION} VERSION_GREATER ${O3DE_MOST_COMPATIBLE_ENGINE_VERSION})
            set(O3DE_MOST_COMPATIBLE_ENGINE_PATH "${manifest_engine_path}")
            set(O3DE_MOST_COMPATIBLE_ENGINE_VERSION ${PACKAGE_VERSION})
            message(VERBOSE "Found more compatible engine '${manifest_engine_path}' with version '${PACKAGE_VERSION}' because it has a greater version number.")
        else()
            message(VERBOSE "Not using engine '${manifest_engine_path}' with version '${PACKAGE_VERSION}' because it doesn't have a greater version number or has a different engine name.")
        endif()
    else()
        message(VERBOSE "Ignoring '${manifest_engine_path}' because it is not a compatible engine.")
    endif()
endforeach()

if(O3DE_MOST_COMPATIBLE_ENGINE_PATH)
    message(STATUS "Selecting engine '${O3DE_MOST_COMPATIBLE_ENGINE_PATH}'")
    # Make sure PACKAGE_VERSION_COMPATIBLE is set so Findo3de.cmake knows
    # compatibility was checked
    set(PACKAGE_VERSION_COMPATIBLE True)
    set(PACKAGE_VERSION O3DE_MOST_COMPATIBLE_ENGINE_VERSION)
    list(APPEND CMAKE_MODULE_PATH "${O3DE_MOST_COMPATIBLE_ENGINE_PATH}/cmake")
    return()
endif()

# No compatible engine was found.
# Read the 'engine' field in project.json or user/project.json for more helpful messages 
if(user_project_json)
    string(JSON user_project_engine ERROR_VARIABLE json_error GET ${user_project_json} engine)
endif()

if(NOT user_project_engine)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/project.json o3de_project_json)
    string(JSON project_engine ERROR_VARIABLE json_error GET ${o3de_project_json} engine)
    if(json_error AND ${project_engine} STREQUAL "NOTFOUND")
        message(FATAL_ERROR "Unable to read key 'engine' from 'project.json'\nError: ${json_error}")
    endif()
endif()

if(user_project_engine)
    message(FATAL_ERROR "The local '${O3DE_USER_PROJECT_JSON_PATH}' engine is '${user_project_engine}' but no compatible engine with that name and version was found.  Please register the compatible engine, or remove the local engine override.\n${registration_error}")
elseif(project_engine)
    message(FATAL_ERROR "The project.json engine is '${project_engine}' but no engine with that name and version was found.\n${registration_error}")
else()
    set(project_registration_error [=[
    Project registration is required before configuring a project.
    Run 'scripts/o3de register -pp PROJECT_PATH --engine-path ENGINE_PATH' from the engine root.
    ]=])
    message(FATAL_ERROR "${project_registration_error}")
endif()
