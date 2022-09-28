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

# Add a GLOBAL property which can be used to quickly determine if a directory is an external subdirectory
get_property(cache_external_subdirs CACHE O3DE_EXTERNAL_SUBDIRS PROPERTY VALUE)
foreach(cache_external_subdir IN LISTS cache_external_subdirs)
    file(REAL_PATH ${cache_external_subdir} real_external_subdir)
    set_property(GLOBAL PROPERTY "O3DE_SUBDIRECTORY_${real_external_subdir}" TRUE)
endforeach()

# The visited_gem_name_set is used to append the external_subdirectories found
# within descendant gems to the parants to the globl O3DE_EXTERNAL_SUBDIRS_GEM_<gem_name> property
# i.e If `GemA` "external_subdirectories" points to `GemB` and `GemB` external_subdirectories
# points to `GemC`.
# Then O3DE_EXTERNAL_SUBDIRS_GEM_GemA = [<AbsPath GemB>, <AbsPath GemC>]
# And O3DE_EXTERNAL_SUBDIRS_GEM_GemB = [<AbsPath GemC>]
#! add_o3de_object_gem_json_external_subdirectories : Recurses through external subdirectories
#! originally found in the add_*_json_external_subdirectories command
function(add_o3de_object_gem_json_external_subdirectories object_type object_name gem_path visited_gem_name_set_ref)
    set(gem_json_path ${gem_path}/gem.json)
    if(EXISTS ${gem_json_path})
        o3de_read_json_external_subdirs(gem_external_subdirs ${gem_path}/gem.json)
        # Read the gem_name from the gem.json and map it to the gem path
        o3de_read_json_key(gem_name "${gem_path}/gem.json" "gem_name")
        if (gem_name)
            set_property(GLOBAL PROPERTY "@GEMROOT:${gem_name}@" "${gem_path}")
        endif()

        # Push the gem name onto the visited set
        list(APPEND ${visited_gem_name_set_ref} ${gem_name})
        foreach(gem_external_subdir IN LISTS gem_external_subdirs)
            file(REAL_PATH ${gem_external_subdir} real_external_subdir BASE_DIRECTORY ${gem_path})

            if(NOT object_name STREQUAL "")
                # Append external subdirectory to the O3DE_EXTERNAL_SUBDIRS_${object_type}_${object_name} PROPERTY
                set(object_external_subdir_property_name O3DE_EXTERNAL_SUBDIRS_${object_type}_${object_name})
            else()
                # Append external subdirectory to the O3DE_EXTERNAL_SUBDIRS_${object_type} PROPERTY
                set(object_external_subdir_property_name O3DE_EXTERNAL_SUBDIRS_${object_type})
            endif()

            get_property(current_external_subdirs GLOBAL PROPERTY ${object_external_subdir_property_name})
            if(NOT real_external_subdir IN_LIST current_external_subdirs)
                set_property(GLOBAL APPEND PROPERTY ${object_external_subdir_property_name} "${real_external_subdir}")
                set_property(GLOBAL PROPERTY "O3DE_SUBDIRECTORY_${real_external_subdir}" TRUE)
                foreach(visited_gem_name IN LISTS ${visited_gem_name_set_ref})
                    # Append the external subdirectories that come with the gem to
                    # the visited_gem_set O3DE_EXTERNAL_SUBDIRS_GEM_<gem-name> properties as well
                    set_property(GLOBAL APPEND PROPERTY O3DE_EXTERNAL_SUBDIRS_GEM_${visited_gem_name} ${real_external_subdir})
                endforeach()
                add_o3de_object_gem_json_external_subdirectories("${object_type}" "${object_name}" "${real_external_subdir}" "${visited_gem_name_set_ref}")
            endif()
        endforeach()
        # Pop the gem name from the visited set
        list(POP_BACK ${visited_gem_name_set_ref})
    endif()
endfunction()

#! add_o3de_object_json_external_subdirectories:
#! Returns the external_subdirectories referenced by the supplied <o3de_object>.json
#! This will recurse through to check for gem.json external_subdirectories
#! via calling the *_gem_json_extenal_subdirectories variant of this function
function(add_o3de_object_json_external_subdirectories object_type object_name object_path object_json_filename)
    set(object_json_path ${object_path}/${object_json_filename})
    if(EXISTS ${object_json_path})
        o3de_read_json_external_subdirs(object_external_subdirs ${object_json_path})
        foreach(object_external_subdir IN LISTS object_external_subdirs)
            file(REAL_PATH ${object_external_subdir} real_external_subdir BASE_DIRECTORY ${object_path})

            # Append external subdirectory ONLY to O3DE_EXTERNAL_SUBDIRS_PROJECT_${project_name} PROPERTY
            if(NOT object_name STREQUAL "")
                # Append external subdirectory to the O3DE_EXTERNAL_SUBDIRS_${object_type}_${object_name} PROPERTY
                set(object_external_subdir_property_name O3DE_EXTERNAL_SUBDIRS_${object_type}_${object_name})
            else()
                # Append external subdirectory to the O3DE_EXTERNAL_SUBDIRS_${object_type} PROPERTY
                set(object_external_subdir_property_name O3DE_EXTERNAL_SUBDIRS_${object_type})
            endif()

            get_property(current_external_subdirs GLOBAL PROPERTY ${object_external_subdir_property_name})
            if(NOT real_external_subdir IN_LIST current_external_subdirs)
                set_property(GLOBAL APPEND PROPERTY ${object_external_subdir_property_name} "${real_external_subdir}")
                set_property(GLOBAL PROPERTY "O3DE_SUBDIRECTORY_${real_external_subdir}" TRUE)
                set(visited_gem_name_set)
                add_o3de_object_gem_json_external_subdirectories("${object_type}" "${object_name}" "${real_external_subdir}" visited_gem_name_set)
            endif()
        endforeach()
    endif()
endfunction()

# The following functions is for gathering the list of external subdirectories
# provided by the engine.json
function(add_engine_json_external_subdirectories)
    add_o3de_object_json_external_subdirectories("ENGINE" "" "${LY_ROOT_FOLDER}" "engine.json")
endfunction()


# The following functions is for gathering the list of external subdirectories
# provided by the project.json
function(add_project_json_external_subdirectories project_path project_name)
    add_o3de_object_json_external_subdirectories("PROJECT" "${project_name}" "${project_path}" "project.json")
endfunction()

#! add_o3de_manifest_json_external_subdirectories : Adds the list of external_subdirectories
#! in the user o3de_manifest.json to the O3DE_EXTERNAL_SUBDIRS_O3DE_MANIFEST property
function(add_o3de_manifest_json_external_subdirectories)
    # Retrieves the path to the o3de_manifest.json(includes the name)
    o3de_get_manifest_path(manifest_path)
    # Separate the o3de_manifest.json from the path to it
    cmake_path(GET manifest_path FILENAME manifest_json_name)
    cmake_path(GET manifest_path PARENT_PATH manifest_path)

    add_o3de_object_json_external_subdirectories("O3DE_MANIFEST" "" "${manifest_path}" "${manifest_json_name}")
endfunction()

#! Gather unique_list of all external subdirectories that is union
#! of the engine.json, project.json, o3de_manifest.json and any gem.json files found visiting
function(get_all_external_subdirectories output_subdirs)
    # Gather user supplied external subdirectories via the Cache Variable
    get_property(all_external_subdirs GLOBAL PROPERTY O3DE_EXTERNAL_SUBDIRS)
    get_property(manifest_external_subdirs GLOBAL PROPERTY O3DE_EXTERNAL_SUBDIRS_O3DE_MANIFEST)
    list(APPEND all_external_subdirs ${manifest_external_subdirs})
    get_property(engine_external_subdirs GLOBAL PROPERTY O3DE_EXTERNAL_SUBDIRS_ENGINE)
    list(APPEND all_external_subdirs ${engine_external_subdirs})

    # Gather the list of every configured project external subdirectory
    # and and append them to the list of external subdirectories
    get_property(project_names GLOBAL PROPERTY O3DE_PROJECTS_NAME)
    foreach(project_name IN LISTS project_names)
        get_property(project_external_subdirs GLOBAL PROPERTY O3DE_EXTERNAL_SUBDIRS_PROJECT_${project_name})
        list(APPEND all_external_subdirs ${project_external_subdirs})
    endforeach()

    list(REMOVE_DUPLICATES all_external_subdirs)
    set(${output_subdirs} ${all_external_subdirs} PARENT_SCOPE)
endfunction()


#! Accepts a list of gem names (which can be read from the project.json, gem.json or engine.json)
#! and a list of ALL registered external subdirectories across all manifest
#! and cross checks them against union of all external subdirectories to determine the gem path.
#! If that gem path exist it is appended to the output parameter output gem directories parameter
#! A fatal error is logged indicating that is not gem could not be found in the list of external subdirectories
function(query_gem_paths_from_external_subdirs output_gem_dirs gem_names registered_external_subdirs)
    if (gem_names)
        foreach(gem_name IN LISTS gem_names)
            unset(gem_path)
            o3de_find_gem_with_registered_external_subdirs(${gem_name} gem_path "${registered_external_subdirs}")
            if (gem_path)
                list(APPEND gem_dirs ${gem_path})
            else()
                list(JOIN registered_external_subdirs "\n" external_subdirs_formatted)
                message(SEND_ERROR "The gem \"${gem_name}\""
                " could not be found in any gem.json from the following list of registered external subdirectories:\n"
                "${external_subdirs_formatted}")
                break()
            endif()
        endforeach()
    endif()
    set(${output_gem_dirs} ${gem_dirs} PARENT_SCOPE)
endfunction()

#! Queries the list of gem names against the list of ALL registered external subdirectories
#! in order to determine the paths corresponding to the gem names
function(add_registered_gems_to_external_subdirs output_gem_dirs gem_names)
    get_all_external_subdirectories(registered_external_subdirs)
    query_gem_paths_from_external_subdirs(gem_dirs "${gem_names}" "${registered_external_subdirs}")
    set(${output_gem_dirs} ${gem_dirs} PARENT_SCOPE)
endfunction()

#! Recurses "dependencies" array if the external subdirectory is a gem(contains a gem.json)
#! for each subdirectory in use.
#! This function looks up the each dependent gem path from the registered external subdirectory set
#! That list of resolved gem paths then have this function invoked on it to perform the same behavior
#! When every descendent gem referenced from the "dependencies" field of the current subdirectory is visited
#! it is then appended to a list of output external subdirectories
#! NOTE: This must be invoked after all the add_*_json_external_subdirectories function
function(reorder_dependent_gems_with_cycle_detection _output_external_dirs subdirs_in_use registered_external_subdirs cycle_detection_set)
    # output_external_dirs is a variable whose value is the name of a variable to set in the parent scope
    # So double resolve the variable to retrieve its value
    set(current_external_dirs "${${_output_external_dirs}}")

    foreach(external_subdir IN LISTS subdirs_in_use)
        # If a cycle is detected, fatal error and output the list of subdirectories that led to the outcome
        if (external_subdir IN_LIST cycle_detection_set)
            message(FATAL_ERROR "While visiting \"${external_subdir}\", a cycle was detected in the \"dependencies\""
            " array of the following gem.json files in the directories: ${cycle_detection_set}")
        endif()
        # This subdirectory has already been processed so skip to the next one
        if(external_subdir IN_LIST current_external_dirs)
            continue()
        endif()

        get_property(ordered_dependent_subdirs GLOBAL PROPERTY "Dependent:${external_subdir}")
        if(ordered_dependent_subdirs)
            # Re-use the cached list of dependent subdirs if available
            list(APPEND current_external_dirs "${ordered_dependent_subdirs}")
        else()
            cmake_path(SET gem_manifest_path "${external_subdir}/gem.json")
            if(EXISTS ${gem_manifest_path})
                # Read the "dependencies" array from gem.json
                o3de_read_json_array(dependencies_array "${gem_manifest_path}" "dependencies")
                # Lookup the paths using the dependent gem names
                unset(reference_external_dirs)
                query_gem_paths_from_external_subdirs(reference_external_dirs "${dependencies_array}" "${registered_external_subdirs}")

                # Append the external subdirectory into the children cycle_detection_set
                set(child_cycle_detection_set ${cycle_detection_set} ${external_subdir})

                # Recursively visit the list of gem dependencies for the current external subdir
                reorder_dependent_gems_with_cycle_detection(current_external_dirs "${reference_external_dirs}"
                    "${registered_external_subdirs}" "${child_cycle_detection_set}")
                # Append the referenced gem directories before the current external subdir so that they are visited first
                list(APPEND current_external_dirs "${reference_external_dirs}")

                # Cache the list of external subdirectories so that it can be reused in subsequent calls
                set_property(GLOBAL PROPERTY "Dependent:${external_subdir}" "${reference_external_dirs}")
            endif()
        endif()

        # Now append the external subdir
        list(APPEND current_external_dirs ${external_subdir})
    endforeach()

    set(${_output_external_dirs} ${current_external_dirs} PARENT_SCOPE)
endfunction()

function(reorder_dependent_gems_before_external_subdirs output_gem_subdirs subdirs_in_use)
    # Lookup the registered external subdirectories once and re-use it for each call
    get_all_external_subdirectories(registered_external_subdirs)
    # Supply an empty visited set and cycle_detection_set argument
    reorder_dependent_gems_with_cycle_detection(output_external_dirs "${subdirs_in_use}" "${registered_external_subdirs}" "")
    set(${output_gem_subdirs} ${output_external_dirs} PARENT_SCOPE)
endfunction()

#! Gather unique_list of all external subdirectories that the o3de object provides or uses
#! The list is made up of the following
#! - The paths of gems referenced in the <o3de_object>.json "gem_names" key. Those paths are queried
#!   from the "external_subdirectories" in o3de_manifest.json
#! - The <o3de_object> path
#! - The list of external_subdirectories found by recursively visting the <o3de_object>.json "external_subdirectories"
function(get_all_external_subdirectories_for_o3de_object output_subdirs object_type object_name object_path object_json_filename)
    # Append the gems referenced by name from "gem_names" field in the <object>.json
    # These gems are registered in the users o3de_manifest.json
    o3de_read_json_array(gem_names ${object_path}/${object_json_filename} "gem_names")
    add_registered_gems_to_external_subdirs(object_gem_reference_dirs "${gem_names}")
    list(APPEND subdirs_for_object ${object_gem_reference_dirs})

    # Also append the array the "external_subdirectories" from each gem referenced through the "gem_names"
    # field
    foreach(gem_name IN LISTS gem_names)
        get_property(gem_real_external_subdirs GLOBAL PROPERTY O3DE_EXTERNAL_SUBDIRS_GEM_${gem_name})
        list(APPEND subdirs_for_object ${gem_real_external_subdirs})
    endforeach()

    # Append the list of external_subdirectories that come with the object
    if(NOT object_name STREQUAL "")
        # query the O3DE_EXTERNAL_SUBDIRS_${object_type}_${object_name} PROPERTY
        set(object_external_subdir_property_name O3DE_EXTERNAL_SUBDIRS_${object_type}_${object_name})
    else()
        # query the O3DE_EXTERNAL_SUBDIRS_${object_type} PROPERTY
        set(object_external_subdir_property_name O3DE_EXTERNAL_SUBDIRS_${object_type})
    endif()

    get_property(object_external_subdirs GLOBAL PROPERTY ${object_external_subdir_property_name})
    list(APPEND subdirs_for_object ${object_external_subdirs})

    list(REMOVE_DUPLICATES subdirs_for_object)
    set(${output_subdirs} ${subdirs_for_object} PARENT_SCOPE)
endfunction()

#! Gather the unqiue list of all external subdirectories that the engine provides("external_subdirectories")
#! plus all external subdirectories that every active project provides("external_subdirectories")
#! or references("gem_names")
function(get_external_subdirectories_in_use output_subdirs)
    # Gather the list of external subdirectories set through the O3DE_EXTERNAL_SUBDIRS Cache Variable
    get_property(all_external_subdirs CACHE O3DE_EXTERNAL_SUBDIRS PROPERTY VALUE)
    # Append the list of external subdirectories from the engine.json
    get_all_external_subdirectories_for_o3de_object(engine_external_subdirs "ENGINE" "" ${LY_ROOT_FOLDER} "engine.json")
    list(APPEND all_external_subdirs ${engine_external_subdirs})

    # Visit each LY_PROJECTS entry and append the external subdirectories
    # the project provides and references
    get_property(O3DE_PROJECTS_NAME GLOBAL PROPERTY O3DE_PROJECTS_NAME)
    foreach(project_name project_path IN ZIP_LISTS O3DE_PROJECTS_NAME LY_PROJECTS)
        file(REAL_PATH ${project_path} full_directory_path BASE_DIRECTORY ${CMAKE_SOURCE_DIR})
        # Append the project root path to the list of external subdirectories so that it is visited
        list(APPEND all_external_subdirs ${project_path})
        get_all_external_subdirectories_for_o3de_object(external_subdirs "PROJECT" "${project_name}" ${full_directory_path} "project.json")
        list(APPEND all_external_subdirs ${external_subdirs})
    endforeach()

    # Make sure any gems in the "dependencies" field of a gem.json
    # are ordered before that gem, so they are parsed first.
    reorder_dependent_gems_before_external_subdirs(all_external_subdirs "${all_external_subdirs}")
    list(REMOVE_DUPLICATES all_external_subdirs)
    set(${output_subdirs} ${all_external_subdirs} PARENT_SCOPE)
endfunction()

#! Visit all external subdirectories that is in use by the engine and each project
#! This visits "external_subdirectories" listed in the engine.json,
#! the "external_subdirectories" listed in the each LY_PROJECTS project.json,
#! and the "external_subdirectories" listed o3de_manifest.json in which the engine.json/project.json
#! references in their "gem_names" key.
function(add_subdirectory_on_external_subdirs)
    # Query the list of external subdirectories in use by the engine and any projects
    get_external_subdirectories_in_use(all_external_subdirs)

    # Log the external subdirectory visit order
    message(VERBOSE "add_subdirectory will be called on the following external subdirectories in order:")
    foreach(external_directory IN LISTS all_external_subdirs)
        message(VERBOSE "${external_directory}")
    endforeach()

    # Loop over the additional external subdirectories and invoke add_subdirectory on them
    foreach(external_directory IN LISTS all_external_subdirs)
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
