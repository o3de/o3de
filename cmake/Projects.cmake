#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Deals with projects (e.g. game gems)

include_guard()

set(LY_PROJECTS "${LY_PROJECTS}" CACHE STRING "List of projects to enable, this can be a relative path to the engine root or an absolute path")

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
# \arg:DEPENDENT_TARGETS additional list of targets should be added as load-time dependencies for the TARGETS list
#
function(ly_add_project_dependencies)

    set(options)
    set(oneValueArgs PROJECT_NAME)
    set(multiValueArgs TARGETS DEPENDENCIES_FILES DEPENDENT_TARGETS)

    cmake_parse_arguments(ly_add_project_dependencies "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments
    if(NOT ly_add_project_dependencies_PROJECT_NAME)
        message(FATAL_ERROR "PROJECT_NAME parameter missing. If not project name is needed, then call ly_add_target_dependencies directly")
    endif()

    ly_add_target_dependencies(
        PREFIX ${ly_add_project_dependencies_PROJECT_NAME}
        TARGETS ${ly_add_project_dependencies_TARGETS}
        DEPENDENCIES_FILES ${ly_add_project_dependencies_DEPENDENCIES_FILES}
        DEPENDENT_TARGETS ${ly_add_project_dependencies_DEPENDENT_TARGETS}
    )
endfunction()


#template for generating the project build_path setreg
set(project_build_path_template [[
{
    "Amazon": {
        "Project": {
            "Settings": {
                "Build": {
                    "project_build_path": "@project_bin_path@"
                }
            }
        }
    }
}]]
)

#! ly_generate_project_build_path_setreg: Generates a .setreg file that contains an absolute path to the ${CMAKE_BINARY_DIR}
#  This allows locate the directory where the project it's binaries are built to be located within the engine.
#  Which are the shared libraries and launcher executables
#  When an a pre-built engine application runs from a directory other than the project build directory, it needs
#  to be able to locate the project build directory to determine the list of gems that the project depends on to load
#  as well the location of those gems.
#  For example if the project uses an external gem not associated with the engine, that gem's dlls/so/dylib files
#  would be located within the project build directory and the engine SDK binary directory would need that info

# NOTE: This only needed for non-monolithic host platforms.
#       This is because there are no dynamic gems to load on monolithic builds
#       Furthermore the pre-built SDK engine applications such as the Editor and AssetProcessor
#       can only run on the host platform
# \arg:project_real_path Full path to the o3de project directory
function(ly_generate_project_build_path_setreg project_real_path)
    # The build path isn't needed on non-monolithic platforms
    # Nor on any non-host platforms
    if (LY_MONOLITHIC_GAME OR NOT PAL_TRAIT_BUILD_HOST_TOOLS)
        return()
    endif()

    # Set the project_bin_path to the ${CMAKE_BINARY_DIR} to provide the configure template
    # with the project build directory
    set(project_bin_path ${CMAKE_BINARY_DIR})
    string(CONFIGURE ${project_build_path_template} project_build_path_setreg_content @ONLY)
    set(project_user_build_path_setreg_file ${project_real_path}/user/Registry/Platform/${PAL_PLATFORM_NAME}/build_path.setreg)
    file(GENERATE OUTPUT ${project_user_build_path_setreg_file} CONTENT ${project_build_path_setreg_content})
endfunction()


function(add_project_json_external_subdirectories project_path)
    set(project_json_path ${project_path}/project.json)
    if(EXISTS ${project_json_path})
        read_json_external_subdirs(external_subdirs ${project_path}/project.json)
        foreach(external_subdir ${external_subdirs})
            file(REAL_PATH ${external_subdir} real_external_subdir BASE_DIRECTORY ${project_path})
            list(APPEND project_external_subdirs ${real_external_subdir})
        endforeach()

        set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS ${project_external_subdirs})
    endif()
endfunction()

# Add the projects here so the above function is found
foreach(project ${LY_PROJECTS})
    file(REAL_PATH ${project} full_directory_path BASE_DIRECTORY ${CMAKE_SOURCE_DIR})
    string(SHA256 full_directory_hash ${full_directory_path})

    # Truncate the full_directory_hash down to 8 characters to avoid hitting the Windows 260 character path limit
    # when the external subdirectory contains relative paths of significant length
    string(SUBSTRING ${full_directory_hash} 0 8 full_directory_hash)

    get_filename_component(project_folder_name ${project} NAME)
    list(APPEND LY_PROJECTS_FOLDER_NAME ${project_folder_name})
    add_subdirectory(${project} "${project_folder_name}-${full_directory_hash}")
    ly_generate_project_build_path_setreg(${full_directory_path})
    add_project_json_external_subdirectories(${full_directory_path})
endforeach()

# If just one project is defined we pass it as a parameter to the applications
list(LENGTH LY_PROJECTS projects_length)
if(projects_length EQUAL "1")
    list(GET LY_PROJECTS 0 project)
    file(REAL_PATH ${project} full_directory_path BASE_DIRECTORY ${CMAKE_SOURCE_DIR})
    ly_set(LY_DEFAULT_PROJECT_PATH ${full_directory_path})
endif()

ly_set(LY_PROJECTS_FOLDER_NAME ${LY_PROJECTS_FOLDER_NAME})
