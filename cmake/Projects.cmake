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

# Deals with projects (e.g. game gems)

include_guard()

set(LY_PROJECTS "" CACHE STRING "List of projects to enable, this can be a relative path to the engine root or an absolute path")

#! ly_add_target_dependencies: adds module load dependencies for this target.
#
#  Each target may have dependencies on gems. To properly define these dependencies, users provides
#  this build target a list of dependencies that need to load dynamically when this target executes
#  So for example, the PythonBindingExample target adds a dependency on the EditorPythonBindings gem
#  via a tool dependencies file
#
# \arg:PREFIX(optional) value to prefix to the generated cmake_dependencies .setreg file for each target
#      found in the DEPENDENCIES_FILES. The PREFIX and each dependent target will be joined using the <dot> symbol
# \arg:TARGETS names of the targets to associate the dependencies to
# \arg:DEPENDENCIES_FILES file(s) that contains the load-time dependencies the TARGETS will be associated to
# \arg:DEPENDENT_TARGETS additional list of targets should be added as load-time dependencies for the TARGETS list
#
function(ly_add_target_dependencies)

    set(options)
    set(oneValueArgs PREFIX)
    set(multiValueArgs TARGETS DEPENDENCIES_FILES DEPENDENT_TARGETS)

    cmake_parse_arguments(ly_add_gem_dependencies "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments
    if(NOT ly_add_gem_dependencies_TARGETS)
        message(FATAL_ERROR "You must provide at least one target to associate the dependencies with")
    endif()

    if(NOT ly_add_gem_dependencies_DEPENDENCIES_FILES AND NOT ly_add_gem_dependencies_DEPENDENT_TARGETS)
        message(FATAL_ERROR "DEPENDENCIES_FILES parameter missing. It must be supplied unless the DEPENDENT_TARGETS parameter is set")
    endif()

    unset(ALL_GEM_DEPENDENCIES)
    foreach(dependency_file ${ly_add_gem_dependencies_DEPENDENCIES_FILES})
    #unset any GEM_DEPENDENCIES and include the dependencies file, that should populate GEM_DEPENDENCIES
    unset(GEM_DEPENDENCIES)
        include(${dependency_file})
        list(APPEND ALL_GEM_DEPENDENCIES ${GEM_DEPENDENCIES})
    endforeach()

    # Append the DEPENDENT_TARGETS to the list of ALL_GEM_DEPENDENCIES
    list(APPEND ALL_GEM_DEPENDENCIES ${ly_add_gem_dependencies_DEPENDENT_TARGETS})

    # for each target, add the dependencies and generate gems json
    foreach(target ${ly_add_gem_dependencies_TARGETS})
        ly_add_dependencies(${target} ${ALL_GEM_DEPENDENCIES})

        # Add the target to the LY_DELAYED_LOAD_DEPENDENCIES if it isn't already on the list
        get_property(delayed_load_target_set GLOBAL PROPERTY LY_DELAYED_LOAD_"${ly_add_gem_dependencies_PREFIX},${target}" SET)
        if(NOT delayed_load_target_set)
            set_property(GLOBAL APPEND PROPERTY LY_DELAYED_LOAD_DEPENDENCIES "${ly_add_gem_dependencies_PREFIX},${target}")
        endif()
        foreach(gem_target ${ALL_GEM_DEPENDENCIES})
            # Add the list of gem dependencies to the LY_TARGET_DELAYED_DEPENDENCIES_${ly_add_gem_dependencies_PREFIX};${target} property
            set_property(GLOBAL APPEND PROPERTY LY_DELAYED_LOAD_"${ly_add_gem_dependencies_PREFIX},${target}" ${gem_target})
        endforeach()
    endforeach()
endfunction()

#! ly_add_project_dependencies: adds the dependencies to runtime and tools for this project.
#
#  Each project may have dependencies to gems. To properly define these dependencies, we are making the project to define
#  through a "files list" cmake file the dependencies to the different targets.
#  So for example, the game's runtime dependencies are associated to the project's launcher; the game's tools dependencies
#  are associated to the asset processor; etc
#
# \arg:PROJECT_NAME name of the game project
# \arg:TARGETS names of the targets to associate the dependencies to
# \arg:DEPENDENCIES_FILES file(s) that contains the runtime dependencies the TARGETS will be associated to
#
function(ly_add_project_dependencies)

    set(options)
    set(oneValueArgs PROJECT_NAME)
    set(multiValueArgs TARGETS DEPENDENCIES_FILES)

    cmake_parse_arguments(ly_add_project_dependencies "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments
    if(NOT ly_add_project_dependencies_PROJECT_NAME)
        message(FATAL_ERROR "PROJECT_NAME parameter missing. If not project name is needed, then call ly_add_target_dependencies directly")
    endif()

    ly_add_target_dependencies(
        PREFIX ${ly_add_project_dependencies_PROJECT_NAME}
        TARGETS ${ly_add_project_dependencies_TARGETS}
        DEPENDENCIES_FILES ${ly_add_project_dependencies_DEPENDENCIES_FILES}
    )
endfunction()

# Add the projects here so the above function is found
foreach(project ${LY_PROJECTS})
    get_filename_component(full_directory_path ${project} REALPATH ${CMAKE_SOURCE_DIR})
    string(SHA256 full_directory_hash ${full_directory_path})

    # Truncate the full_directory_hash down to 8 characters to avoid hitting the Windows 260 character path limit
    # when the external subdirectory contains relative paths of significant length
    string(SUBSTRING ${full_directory_hash} 0 8 full_directory_hash)

    get_filename_component(project_folder_name ${project} NAME)
    list(APPEND LY_PROJECTS_FOLDER_NAME ${project_folder_name})
    add_subdirectory(${project} "${project_folder_name}-${full_directory_hash}")
endforeach()
ly_set(LY_PROJECTS_FOLDER_NAME ${LY_PROJECTS_FOLDER_NAME})
