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

# Read the 'engine' field in project.json for compatibility with older engines
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/project.json o3de_project_json)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/project.json)
string(JSON O3DE_ENGINE_NAME_TO_USE ERROR_VARIABLE json_error GET ${o3de_project_json} engine)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'engine' from 'project.json'\nError: ${json_error}")
endif()

# Allow the user to override the 'engine' field locally using <project>/user/project.json
cmake_path(SET O3DE_USER_PROJECT_JSON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/user/project.json)
if(EXISTS "${O3DE_USER_PROJECT_JSON_PATH}")
    file(READ "${O3DE_USER_PROJECT_JSON_PATH}" user_project_json)
    set_property(GLOBAL PROPERTY O3DE_USER_PROJECT_JSON ${user_project_json})
    string(JSON user_project_engine ERROR_VARIABLE json_error GET ${user_project_json} engine)
    if(NOT json_error)
        # Add cmake configure dependency on user/project.json in case the engine overrides change 
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${O3DE_USER_PROJECT_JSON_PATH}")
        if(user_project_engine)
            # Update the project json data so it is available in the O3DE_PROJECT_JSON global property
            string(JSON o3de_project_json SET "${o3de_project_json}" engine "\"${user_project_engine}\"" )
            set(project_engine ${O3DE_ENGINE_NAME_TO_USE})
            set(O3DE_ENGINE_NAME_TO_USE ${user_project_engine})
        endif()
    endif()
endif()

# Save the project json data with overrides in a global property to avoid performance hit
# from loading the file multiple times
set_property(GLOBAL PROPERTY O3DE_PROJECT_JSON ${o3de_project_json})

# For backwards compatibility
set(LY_ENGINE_NAME_TO_USE ${O3DE_ENGINE_NAME_TO_USE})

# Option 1: Use engine manually set in CMAKE_MODULE_PATH, old engines will check engine name only
find_package(o3de MODULE QUIET)
if(o3de_FOUND)
    message(STATUS "Selecting engine from CMAKE_MODULE_PATH '${CMAKE_MODULE_PATH}'.")
    return()
endif()

# Option 2: Use the engine from the 'engine_path' field in <project>/user/project.json
if(user_project_json)
    string(JSON user_project_engine_path ERROR_VARIABLE json_error GET ${user_project_json} engine_path)
    if(user_project_engine_path AND NOT json_error)
        if(EXISTS "${user_project_engine_path}/cmake/o3deConfigVersion.cmake")
            # Set o3de_DIR or find_package will not search the single path we specify
            set(o3de_DIR "${engine_path}/cmake")
            # Provide a dummy version number '0' or cmake will not consider PACKAGE_VERSION_COMPATIBLE 
            find_package(o3de 0 QUIET CONFIG PATHS "${user_project_engine_path}/cmake" NO_DEFAULT_PATH)
            if(o3de_FOUND AND o3de_VERSION)
                message(STATUS "Selecting engine '${user_project_engine_path}' from 'engine_path' in '<project>/user/project.json'.")
                list(APPEND CMAKE_MODULE_PATH "${user_project_engine_path}/cmake")
                return()
            else()
                message(WARNING "Not using engine at '${user_project_engine_path}' from 'engine_path' in '${O3DE_USER_PROJECT_JSON_PATH}' because it is not compatible with this project.")
            endif()
        else()
            message(WARNING "Not using engine at '${user_project_engine_path}' because ${user_project_engine_path}/cmake/o3deConfigVersion.cmake was not found")
        endif()
    endif()
endif()


# Option 3: Find a compatible engine registered in o3de_manfiest.json 
if(DEFINED ENV{USERPROFILE} AND EXISTS $ENV{USERPROFILE})
    set(manifest_path $ENV{USERPROFILE}/.o3de/o3de_manifest.json) # Windows
else()
    set(manifest_path $ENV{HOME}/.o3de/o3de_manifest.json) # Unix
endif()

set(registration_error [=[
Engine registration is required before configuring a project.
Run 'scripts/o3de register --this-engine' from the engine root.
]=])


set(O3DE_ENGINE_PATHS "")
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
        string(JSON engine_path ERROR_VARIABLE json_error GET ${manifest_json} engines "${array_index}")
        if(json_error)
            message(FATAL_ERROR "Unable to read 'engines/${array_index}' from '${manifest_path}'\nError: ${json_error}\n${registration_error}")
        endif()
        list(APPEND O3DE_ENGINE_PATHS ${engine_path})
    endforeach()
elseif(NOT CMAKE_MODULE_PATH)
    message(FATAL_ERROR "O3DE Manifest file not found at '${manifest_path}'.\n${registration_error}")
endif()

# We cannot just run find_package on the list of engine paths because
# CMAKE_FIND_PACKAGE_SORT_ORDER sorts based on file name so we cannot   
# guarantee engine paths will be sorted correctly by version 
set(O3DE_MOST_COMPATIBLE_ENGINE_PATH "")
set(O3DE_MOST_COMPATIBLE_ENGINE_VERSION "")

foreach(engine_path IN LISTS O3DE_ENGINE_PATHS) 
    # Set o3de_DIR or find_package will not search the single path we specify 
    set(o3de_DIR "${engine_path}/cmake") 
    
    # when the CONFIG option is specified find_package will look for o3deConfigVersion.cmake 
    # Provide a dummy version number '0' or cmake will not consider PACKAGE_VERSION_COMPATIBLE 
    find_package(o3de 0 QUIET CONFIG PATHS "${engine_path}/cmake" NO_DEFAULT_PATH) 

    if(o3de_FOUND) 
        if(NOT O3DE_MOST_COMPATIBLE_ENGINE_PATH) 
            set(O3DE_MOST_COMPATIBLE_ENGINE_PATH "${engine_path}") 
            set(O3DE_MOST_COMPATIBLE_ENGINE_VERSION ${o3de_VERSION}) 
            message(VERBOSE "Found compatible engine '${engine_path}' with version '${o3de_VERSION}'")
        elseif(${o3de_VERSION} VERSION_GREATER ${O3DE_MOST_COMPATIBLE_ENGINE_VERSION})
            set(O3DE_MOST_COMPATIBLE_ENGINE_PATH "${engine_path}")
            set(O3DE_MOST_COMPATIBLE_ENGINE_VERSION ${o3de_VERSION})
            message(VERBOSE "Found more compatible engine '${engine_path}' with version '${o3de_VERSION}' because it has a greater version number.")
        else()
            message(VERBOSE "Not using engine '${engine_path}' with version '${o3de_VERSION}' because it doesn't have a greater version number or has a different engine name.")
        endif()
    elseif(NOT EXISTS "${engine_path}/cmake/o3deConfigVersion.cmake") 
        message(VERBOSE "Ignoring '${engine_path}' because it is missing an o3deConfigVersion.cmake file.")
    else()
        message(VERBOSE "Ignoring '${engine_path}' because it is not a compatible engine.")
    endif()
endforeach()

if(NOT O3DE_MOST_COMPATIBLE_ENGINE_PATH)
    message(VERBOSE "No compatible engine found, looking for older engines by name only.")

    # look for older engines by name only
    foreach(engine_path IN LISTS O3DE_ENGINE_PATHS)
        set(CMAKE_MODULE_PATH "${engine_path}/cmake")
        find_package(o3de MODULE QUIET)
        if(o3de_FOUND)
            message(STATUS "Selecting engine '${engine_path}' with compatible engine name")
            return()
        else()
            message(VERBOSE "Didn't find compatible older engine at '${engine_path}'")
        endif()
    endforeach()

    if(user_project_engine)
        message(FATAL_ERROR "The local '${O3DE_USER_PROJECT_JSON_PATH}' engine is '${O3DE_ENGINE_NAME_TO_USE}' but no engine with that name or compatible version has been registered.  Please register the compatible engine, or remove the local engine override.\n${registration_error}")
    else()
        message(FATAL_ERROR "The project.json engine is '${O3DE_ENGINE_NAME_TO_USE}' but no engine with that name or compatible version has been registered.\n${registration_error}")
    endif()
endif()

message(STATUS "Selecting engine '${O3DE_MOST_COMPATIBLE_ENGINE_PATH}'")
list(APPEND CMAKE_MODULE_PATH "${O3DE_MOST_COMPATIBLE_ENGINE_PATH}/cmake")