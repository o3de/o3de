#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include_guard()

################################################################################
# Subdirectory processing
################################################################################

# this function is building up the LY_EXTERNAL_SUBDIRS global property
function(add_engine_gem_json_external_subdirectories gem_path)
    set(gem_json_path ${gem_path}/gem.json)
    if(EXISTS ${gem_json_path})
        read_json_external_subdirs(gem_external_subdirs ${gem_path}/gem.json)
        foreach(gem_external_subdir ${gem_external_subdirs})
            file(REAL_PATH ${gem_external_subdir} real_external_subdir BASE_DIRECTORY ${gem_path})

            # Append external subdirectory if it is not in global property
            get_property(current_external_subdirs GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS)
            if(NOT real_external_subdir IN_LIST current_external_subdirs)
                set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS ${real_external_subdir})
                # Also append the project external subdirectores to the LY_EXTERNAL_SUBDIRS_ENGINE property
                set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS_ENGINE ${real_external_subdir})
                add_engine_gem_json_external_subdirectories(${real_external_subdir})
            endif()
        endforeach()
    endif()
endfunction()

function(add_engine_json_external_subdirectories)
    set(engine_json_path ${LY_ROOT_FOLDER}/engine.json)
    if(EXISTS ${engine_json_path})
        read_json_external_subdirs(engine_external_subdirs ${engine_json_path})
        foreach(engine_external_subdir ${engine_external_subdirs})
            file(REAL_PATH ${engine_external_subdir} real_external_subdir BASE_DIRECTORY ${LY_ROOT_FOLDER})

            # Append external subdirectory if it is not in global property
            get_property(current_external_subdirs GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS)
            if(NOT real_external_subdir IN_LIST current_external_subdirs)
                set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS ${real_external_subdir})
                # Also append the project external subdirectores to the LY_EXTERNAL_SUBDIRS_ENGINE property
                set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS_ENGINE ${real_external_subdir})
                add_engine_gem_json_external_subdirectories(${real_external_subdir})
            endif()
        endforeach()
    endif()
endfunction()


function(add_project_gem_json_external_subdirectories gem_path project_name)
    set(gem_json_path ${gem_path}/gem.json)
    if(EXISTS ${gem_json_path})
        read_json_external_subdirs(gem_external_subdirs ${gem_path}/gem.json)
        foreach(gem_external_subdir ${gem_external_subdirs})
            file(REAL_PATH ${gem_external_subdir} real_external_subdir BASE_DIRECTORY ${gem_path})

            # Append external subdirectory if it is not in global property
            get_property(current_external_subdirs GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS)
            if(NOT real_external_subdir IN_LIST current_external_subdirs)
                set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS ${real_external_subdir})
                # Also append the project external subdirectores to the LY_EXTERNAL_SUBDIRS_${project_name} property
                set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS_${project_name} ${real_external_subdir})
                add_project_gem_json_external_subdirectories(${real_external_subdir} "${project_name}")
            endif()
        endforeach()
    endif()
endfunction()

function(add_project_json_external_subdirectories project_path project_name)
    set(project_json_path ${project_path}/project.json)
    if(EXISTS ${project_json_path})
        read_json_external_subdirs(project_external_subdirs ${project_path}/project.json)
        foreach(project_external_subdir ${project_external_subdirs})
            file(REAL_PATH ${project_external_subdir} real_external_subdir BASE_DIRECTORY ${project_path})

            # Append external subdirectory if it is not in global property
            get_property(current_external_subdirs GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS)
            if(NOT real_external_subdir IN_LIST current_external_subdirs)
                set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS ${real_external_subdir})
                # Also append the project external subdirectores to the LY_EXTERNAL_SUBDIRS_${project_name} property
                set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS_${project_name} ${real_external_subdir})
                add_project_gem_json_external_subdirectories(${real_external_subdir} "${project_name}")
            endif()
        endforeach()
    endif()
endfunction()


#! add_o3de_manifest_gem_json_external_subdirectories : Recurses through external subdirectories
#! originally found in the add_o3de_manifest_json_external_subdirectories command
function(add_o3de_manifest_gem_json_external_subdirectories gem_path)
    set(gem_json_path ${gem_path}/gem.json)
    if(EXISTS ${gem_json_path})
        read_json_external_subdirs(gem_external_subdirs ${gem_path}/gem.json)
        foreach(gem_external_subdir ${gem_external_subdirs})
            file(REAL_PATH ${gem_external_subdir} real_external_subdir BASE_DIRECTORY ${gem_path})

            # Append external subdirectory ONLY to LY_EXTERNAL_SUBDIRS_O3DE_MANIFEST PROPERTY
            # It is not appended to LY_EXTERNAL_SUBDIRS unless that gem is used by the project
            get_property(current_external_subdirs GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS_O3DE_MANIFEST)
            if(NOT real_external_subdir IN_LIST current_external_subdirs)
                set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS_O3DE_MANIFEST ${real_external_subdir})
                add_o3de_manifest_gem_json_external_subdirectories(${real_external_subdir})
            endif()
        endforeach()
    endif()
endfunction()

#! add_o3de_manifest_json_external_subdirectories : Adds the list of external_subdirectories
#! in the user o3de_manifest.json to the LY_EXTERNAL_SUBDIRS_O3DE_MANIFEST property
function(add_o3de_manifest_json_external_subdirectories)
    o3de_get_manifest_path(manifest_path)
    if(EXISTS ${manifest_path})
        read_json_external_subdirs(o3de_manifest_external_subdirs ${manifest_path})
        foreach(manifest_external_subdir ${o3de_manifest_external_subdirs})
            file(REAL_PATH ${manifest_external_subdir} real_external_subdir BASE_DIRECTORY ${LY_ROOT_FOLDER})

            # Append external subdirectory ONLY to LY_EXTERNAL_SUBDIRS_O3DE_MANIFEST PROPERTY
            # It is not appended to LY_EXTERNAL_SUBDIRS unless that gem is used by the project
            get_property(current_external_subdirs GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS_O3DE_MANIFEST)
            if(NOT real_external_subdir IN_LIST current_external_subdirs)
                set_property(GLOBAL APPEND PROPERTY LY_EXTERNAL_SUBDIRS_O3DE_MANIFEST ${real_external_subdir})
                add_o3de_manifest_gem_json_external_subdirectories(${real_external_subdir})
            endif()
        endforeach()
    endif()
endfunction()

#! Gather unique_list of all external subdirectories that is union
#! of the engine.json, project.json, o3de_manifest.json and any gem.json files found visiting
function(get_all_external_subdirectories output_subdirs)
    get_property(all_external_subdirs GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS)
    get_property(manifest_external_subdirs GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS_O3DE_MANIFEST)
    list(APPEND all_external_subdirs ${manifest_external_subdirs})
    list(REMOVE_DUPLICATES all_external_subdirs)
    set(${output_subdirs} ${all_external_subdirs} PARENT_SCOPE)
endfunction()

#! add_registered_gems_to_external_subdirs:
#! Accepts a list of gem_names (which can be read from the project.json or engine.json)
#! and cross checks them against union of all external subdirectories to determine the gem path.
#! If that gem exist it is appended to LY_EXTERNAL_SUBDIRS so that that the build generator
#! adds to the generated build project.
#! Otherwise a fatal error is logged indicating that is not gem could not be found in the list of external subdirectories
function(add_registered_gems_to_external_subdirs gem_names)
    if (gem_names)
        get_all_external_subdirectories(all_external_subdirs)
        foreach(gem_name IN LISTS gem_names)
            unset(gem_path)
            o3de_find_gem(${gem_name} gem_path)
            if (gem_path)
                set_property(GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS ${gem_path} APPEND)
            else()
                list(JOIN all_external_subdirs "\n" external_subdirs_formatted)
                message(SEND_ERROR "The gem \"${gem_name}\" from the \"gem_names\" field in the engine.json/project.json "
                " could not be found in any gem.json from the following list of registered external subdirectories:\n"
                "${external_subdirs_formatted}")
                break()
            endif()
        endforeach()
    endif()
endfunction()

function(add_subdirectory_on_external_subdirs)
    # Lookup the paths of "gem_names" array all project.json files and engine.json
    # and append them to the LY_EXTERNAL_SUBDIRS property
    foreach(project ${LY_PROJECTS})
        file(REAL_PATH ${project} full_directory_path BASE_DIRECTORY ${CMAKE_SOURCE_DIR})
        o3de_read_json_array(gem_names ${full_directory_path}/project.json "gem_names")
        add_registered_gems_to_external_subdirs("${gem_names}")
    endforeach()
    o3de_read_json_array(gem_names ${LY_ROOT_FOLDER}/engine.json "gem_names")
    add_registered_gems_to_external_subdirs("${gem_names}")

    get_property(external_subdirs GLOBAL PROPERTY LY_EXTERNAL_SUBDIRS)
    list(APPEND LY_EXTERNAL_SUBDIRS ${external_subdirs})
    list(REMOVE_DUPLICATES LY_EXTERNAL_SUBDIRS)
    # Loop over the additional external subdirectories and invoke add_subdirectory on them
    foreach(external_directory ${LY_EXTERNAL_SUBDIRS})
        # Hash the external_directory name and append it to the Binary Directory section of add_subdirectory
        # This is to deal with potential situations where multiple external directories has the same last directory name
        # For example if D:/Company1/RayTracingGem and F:/Company2/Path/RayTracingGem were both added as a subdirectory
        file(REAL_PATH ${external_directory} full_directory_path)
        string(SHA256 full_directory_hash ${full_directory_path})
        # Truncate the full_directory_hash down to 8 characters to avoid hitting the Windows 260 character path limit
        # when the external subdirectory contains relative paths of significant length
        string(SUBSTRING ${full_directory_hash} 0 8 full_directory_hash)
        # Use the last directory as the suffix path to use for the Binary Directory
        cmake_path(GET external_directory FILENAME directory_name)
        add_subdirectory(${external_directory} ${CMAKE_BINARY_DIR}/External/${directory_name}-${full_directory_hash})
    endforeach()
endfunction()
