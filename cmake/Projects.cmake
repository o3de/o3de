#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Deals with projects (e.g. game gems)

include_guard()

# Passing ${LY_PROJECTS} as the default since in project-centric LY_PROJECTS is defined by the project and
# we want to pick up that one as the value of the variable.
# Ideally this cache variable would be defined before the project sets LY_PROJECTS, but that would mean 
# it would have to be defined in each project.
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

    # for each target, add the dependencies and generate setreg json with the list of gems to load
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
    file(GENERATE OUTPUT ${project_user_build_path_setreg_file} CONTENT "${project_build_path_setreg_content}")
endfunction()

function(install_project_asset_artifacts project_real_path)
    # The cmake tar command has a bit of a flaw
    # Any paths within the archive files it creates are relative to the current working directory.
    # That means with the setup of:
    # cwd = "<project-path>/Cache/pc"
    # project product assets = "<project-path>/Cache/pc/*"
    # cmake dependency registry files  = "<project-path>/build/bin/Release/Registry/*"
    # Running the tar command would result in the assets being placed in the to layout
    # correctly, but the registry files
    # engine.pak/
    #    ../...build/bin/Release/Registry/cmake_dependencies.*.setreg -> Not correct
    #    project.json -> Correct

    # Generate pak for project in release installs
    cmake_path(RELATIVE_PATH CMAKE_RUNTIME_OUTPUT_DIRECTORY BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE install_base_runtime_output_directory)
    set(install_engine_pak_template [=[
if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    set(install_output_folder "${CMAKE_INSTALL_PREFIX}/@install_base_runtime_output_directory@/@PAL_PLATFORM_NAME@/${CMAKE_INSTALL_CONFIG_NAME}/@LY_BUILD_PERMUTATION@")
    set(install_pak_output_folder "${install_output_folder}/Cache/@LY_ASSET_DEPLOY_ASSET_TYPE@")
    set(runtime_output_directory_RELEASE @CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE@)
    if(NOT DEFINED LY_ASSET_DEPLOY_ASSET_TYPE)
        set(LY_ASSET_DEPLOY_ASSET_TYPE @LY_ASSET_DEPLOY_ASSET_TYPE@)
    endif()
    message(STATUS "Generating ${install_pak_output_folder}/engine.pak from @project_real_path@/Cache/${LY_ASSET_DEPLOY_ASSET_TYPE}")
    file(MAKE_DIRECTORY "${install_pak_output_folder}")
    cmake_path(SET cache_product_path "@project_real_path@/Cache/${LY_ASSET_DEPLOY_ASSET_TYPE}")
    # Copy the generated cmake_dependencies.*.setreg files for loading gems in non-monolithic to the cache
    file(GLOB gem_source_paths_setreg "${runtime_output_directory_RELEASE}/Registry/*.setreg")
    # The MergeSettingsToRegistry_TargetBuildDependencyRegistry function looks for lowercase "registry" directory
    file(MAKE_DIRECTORY "${cache_product_path}/registry")
    file(COPY ${gem_source_paths_setreg} DESTINATION "${cache_product_path}/registry")

    file(GLOB product_assets "${cache_product_path}/*")
    list(APPEND pak_artifacts ${product_assets})
    if(pak_artifacts)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar "cf" "${install_pak_output_folder}/engine.pak" --format=zip -- ${pak_artifacts}
            WORKING_DIRECTORY "${cache_product_path}"
            RESULT_VARIABLE archive_creation_result
        )
        if(archive_creation_result EQUAL 0)
            message(STATUS "${install_output_folder}/engine.pak generated")
        endif()
    endif()

    # Remove copied .setreg files from the Cache directory
    unset(artifacts_to_remove)
    foreach(gem_source_path_setreg IN LISTS gem_source_paths_setreg)
        cmake_path(GET gem_source_path_setreg FILENAME setreg_filename)
        list(APPEND artifacts_to_remove "${cache_product_path}/registry/${setreg_filename}")
    endforeach()
    if (artifacts_to_remove)
        file(REMOVE ${artifacts_to_remove})
    endif()
endif()
]=])

    string(CONFIGURE "${install_engine_pak_template}" install_engine_pak_code @ONLY)
    ly_install_run_code("${install_engine_pak_code}")
endfunction()

# Add the projects here so the above function is found
foreach(project ${LY_PROJECTS})
    file(REAL_PATH ${project} full_directory_path BASE_DIRECTORY ${CMAKE_SOURCE_DIR})
    string(SHA256 full_directory_hash ${full_directory_path})

    # Truncate the full_directory_hash down to 8 characters to avoid hitting the Windows 260 character path limit
    # when the external subdirectory contains relative paths of significant length
    string(SUBSTRING ${full_directory_hash} 0 8 full_directory_hash)

    cmake_path(GET project FILENAME project_folder_name )
    list(APPEND LY_PROJECTS_FOLDER_NAME ${project_folder_name})
    # Generate a setreg file with the path to cmake binary directory
    # into <project-path>/user/Registry/Platform/<Platform> directory
    ly_generate_project_build_path_setreg(${full_directory_path})

    # Get project name
    o3de_read_json_key(project_name ${full_directory_path}/project.json "project_name")

    # Set the project name into the global O3DE_PROJECTS_NAME property
    set_property(GLOBAL APPEND PROPERTY O3DE_PROJECTS_NAME ${project_name})

    # Append the project external directory to LY_EXTERNAL_SUBDIR_${project_name} property
    add_project_json_external_subdirectories(${full_directory_path} "${project_name}")

    install_project_asset_artifacts(${full_directory_path})

endforeach()

# If just one project is defined we pass it as a parameter to the applications
list(LENGTH LY_PROJECTS projects_length)
if(projects_length EQUAL "1")
    list(GET LY_PROJECTS 0 project)
    file(REAL_PATH ${project} full_directory_path BASE_DIRECTORY ${CMAKE_SOURCE_DIR})
    ly_set(LY_DEFAULT_PROJECT_PATH ${full_directory_path})
endif()

ly_set(LY_PROJECTS_FOLDER_NAME ${LY_PROJECTS_FOLDER_NAME})
