#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Responsible for generating a settings registry file containing the moduleload dependencies of any ly_delayed_load_targets
# This is used for example, to allow Open 3D Engine Applications to know which set of gems have built for a particular project/target
# combination

include_guard()

#templates used to generate the dependencies setreg files

set(root_dependencies_setreg_template [[
{
@gem_metadata_objects@
}
]]
)
set(gems_json_template [[
    "O3DE": {
        "Gems": {
@gem_setreg_objects@
        }
    }]]
)
string(APPEND gem_module_template
[=[                    "@stripped_gem_target@": {]=] "\n"
[=[$<$<IN_LIST:$<TARGET_PROPERTY:@gem_target@,TYPE>,MODULE_LIBRARY$<SEMICOLON>SHARED_LIBRARY>:                        "Modules":["$<TARGET_FILE_NAME:@gem_target@>"]]=] "\n>"
[=[                    }]=]
)

string(APPEND gem_name_template
[=[            "@gem_name@": {]=] "\n"
[=[                "Targets": {]=] "\n"
[=[@gem_target_objects@]=] "\n"
[=[                }]=] "\n"
[=[            }]=]
)

#!ly_detect_cycle_through_visitation: Detects if there is a cycle based on a list of visited
# items. If the passed item is in the list, then there is a cycle.
# \arg:item - item being checked for the cycle
# \arg:visited_items - list of visited items
# \arg:visited_items_var - list of visited items variable, "item" will be added to the list
# \arg:cycle(variable) - empty string if there is no cycle (an empty string in cmake evaluates
#     to false). If there is a cycle a cycle dependency string detailing the sequence of items
#     that produce a cycle, e.g. A --> B --> C --> A
#
function(ly_detect_cycle_through_visitation item visited_items visited_items_var cycle)
    if(item IN_LIST visited_items)
        unset(dependency_cycle_loop)
        foreach(visited_item IN LISTS visited_items)
            string(APPEND dependency_cycle_loop ${visited_item})
            if(visited_item STREQUAL item)
                string(APPEND dependency_cycle_loop " (cycle starts)")
            endif()
            string(APPEND dependency_cycle_loop " --> ")
        endforeach()
        string(APPEND dependency_cycle_loop "${item} (cycle ends)")
        set(${cycle} "${dependency_cycle_loop}" PARENT_SCOPE)
    else()
        set(cycle "" PARENT_SCOPE) # no cycles
    endif()
    list(APPEND visited_items ${item})
    set(${visited_items_var} "${visited_items}" PARENT_SCOPE)
endfunction()

#!o3de_get_gem_load_dependencies: Retrieves the list of "load" dependencies for a target
# Visits through MANUALLY_ADDED_DEPENDENCIES of targets with a GEM_MODULE property
# to determine which gems a target needs to load
#
# NOTE: ly_get_runtime_dependencies cannot be used as it will recurse through non-manually added dependencies
# to add manually added added which results in false load dependencies.
# \arg:GEM_LOAD_DEPENDENCIES(name) - Output variable to be populated gem load dependencies
# \arg:TARGET(TARGET) - CMake target to examine for dependencies
# \arg:CYCLE_DETECTION_SET(variable name)[optional] - Used to track cycles in load dependencies
function(o3de_get_gem_load_dependencies)
    set(options)
    set(oneValueArgs TARGET GEM_LOAD_DEPENDENCIES_VAR)
    set(multiValueArgs CYCLE_DETECTION_SET)

    cmake_parse_arguments(o3de_get_gem_load_dependencies "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(NOT TARGET ${o3de_get_gem_load_dependencies_TARGET})
        return() # Nothing to do
    endif()

    # Define the indent level variable to 0 in the first iteration
    if(NOT DEFINED indent_level)
        set(indent_level "0")
    endif()

    # Set indent string for current indent level
    string(REPEAT " " "${indent_level}" indent)

    set(target ${o3de_get_gem_load_dependencies_TARGET})
    set(gem_load_dependencies_var ${o3de_get_gem_load_dependencies_GEM_LOAD_DEPENDENCIES_VAR})
    set(cycle_detection_targets ${o3de_get_gem_load_dependencies_CYCLE_DETECTION_SET})

    # Optimize the search by caching gem load dependencies
    get_property(are_dependencies_cached GLOBAL PROPERTY "LY_GEM_LOAD_DEPENDENCIES,${target}" SET)
    if(are_dependencies_cached)
        # We already walked through this target
        get_property(cached_dependencies GLOBAL PROPERTY "LY_GEM_LOAD_DEPENDENCIES,${target}")
        set(${gem_load_dependencies_var} ${cached_dependencies} PARENT_SCOPE)
        message(DEBUG "${indent}Found cache load dependencies for Gem Target \"${target}\": ${cached_dependencies}")
        return()
    endif()

    message(DEBUG "${indent}Visiting target \"${target}\" when looking for gem module load dependencies")

    # detect cycles
    unset(cycle_detected)
    ly_detect_cycle_through_visitation(${target} "${cycle_detection_targets}" cycle_detection_targets cycle_detected)
    if(cycle_detected)
        message(FATAL_ERROR "Runtime dependency detected: ${cycle_detected}")
    endif()

    # For load dependencies, we want to copy over the dependency and traverse them
    # Only if the dependency has a GEM_MODULE property
    get_property(manually_added_dependencies TARGET ${target} PROPERTY MANUALLY_ADDED_DEPENDENCIES)
    unset(load_dependencies)
    if(manually_added_dependencies)
        list(APPEND load_dependencies ${manually_added_dependencies})
        message(VERBOSE "${indent}Gem Target \"${target}\" has direct manually added dependencies of: ${manually_added_dependencies}")
    endif()

    # unset all_gem_load_dependencies to prevent using variable values from the PARENT_SCOPE
    unset(all_gem_load_dependencies)
    # Remove duplicate load dependencies
    list(REMOVE_DUPLICATES load_dependencies)
    foreach(load_dependency IN LISTS load_dependencies)
        # Skip wrapping produced when targets are not created in the same directory
        ly_de_alias_target(${load_dependency} dealias_load_dependency)
        get_property(is_gem_target TARGET ${dealias_load_dependency} PROPERTY GEM_MODULE SET)
        # If the dependency is a "gem module" then add it as a load dependencies
        # and recurse into its manually added dependencies
        if (is_gem_target)
            # shift identation level for recursive calls to ${CMAKE_CURRENT_FUNCTION}
            set(current_indent_level "${indent_level}")
            math(EXPR indent_level "${current_indent_level} + 2")
            unset(dependencies)
            o3de_get_gem_load_dependencies(
                GEM_LOAD_DEPENDENCIES_VAR dependencies
                TARGET ${dealias_load_dependency}
                CYCLE_DETECTION_SET "${cycle_detection_set}"
            )
            # restore indentation level
            set(indent_level "${current_indent_level}")

            list(APPEND all_gem_load_dependencies ${dependencies})
            list(APPEND all_gem_load_dependencies ${dealias_load_dependency})
        endif()
    endforeach()

    list(REMOVE_DUPLICATES all_gem_load_dependencies)
    set_property(GLOBAL PROPERTY "LY_GEM_LOAD_DEPENDENCIES,${target}" "${all_gem_load_dependencies}")
    set(${gem_load_dependencies_var} ${all_gem_load_dependencies} PARENT_SCOPE)
    message(VERBOSE "${indent}Gem Target \"${target}\" has load dependencies of: ${all_gem_load_dependencies}")

endfunction()

#!o3de_get_gem_root_from_target: Uses the supplied gem_target to lookup the nearest
# gem.json file at an ancestor of targets SOURCE_DIR property
#
# \arg:gem_target(TARGET) - Target to look upwards from using its SOURCE_DIR property
function(o3de_get_gem_root_from_target output_gem_root output_gem_name gem_target)
    unset(${output_gem_root} PARENT_SCOPE)
    get_property(gem_source_dir TARGET ${gem_target} PROPERTY SOURCE_DIR)

    # the o3de_find_ancestor_gem_root looks up the nearest gem root path
    # at or above the current directory visited by cmake
    o3de_find_ancestor_gem_root(gem_source_dir gem_name "${gem_source_dir}")

    # Set the gem module root output directory to the location with the gem.json file within it or
    # the supplied gem_target SOURCE_DIR location if no gem.json file was found
    set(${output_gem_root} ${gem_source_dir} PARENT_SCOPE)

    # Set the gem name output value to the name of the gem as in the gem.json file
    if(gem_name)
        set(${output_gem_name} ${gem_name} PARENT_SCOPE)
    endif()
endfunction()

#!ly_populate_gem_objects: Creates a mapping of gem name -> structure of gem module targets and source paths
#
# \arg:output_gem_setreg_object- Variable to populate with configured gem_name_template for each gem dependency
function(ly_populate_gem_objects output_gem_setreg_objects all_gem_dependencies)
    unset(gem_name_list)
    foreach(gem_target ${all_gem_dependencies})
        unset(gem_relative_source_dir)
        # Create path from the LY_ROOT_FOLDER to the the Gem directory
        if (NOT TARGET ${gem_target})
            message(FATAL_ERROR "Dependency ${gem_target} from ${target} does not exist")
        endif()

        o3de_get_gem_root_from_target(gem_module_root gem_name_value ${gem_target})
        if (NOT gem_module_root)
            # If the target doesn't have a gem.json, skip it
            continue()
        endif()

        if (NOT gem_name_value IN_LIST gem_name_list)
            list(APPEND gem_name_list ${gem_name_value})
        endif()

        # De-alias namespace from gem targets before configuring them into the json template
        ly_de_alias_target(${gem_target} stripped_gem_target)
        string(CONFIGURE "${gem_module_template}" gem_module_json @ONLY)

        # Create a "mapping" of gem_name to gem module configured object
        list(APPEND gem_root_target_objects_${gem_name_value} ${gem_module_json})
    endforeach()

    unset(gem_setreg_objects)
    foreach(gem_name IN LISTS gem_name_list)
        # Set the values for the gem_name_template
        set(gem_target_objects ${gem_root_target_objects_${gem_name}})
        list(JOIN gem_target_objects ",\n" gem_target_objects)
        string(CONFIGURE "${gem_name_template}" gem_setreg_object @ONLY)
        list(APPEND gem_setreg_objects ${gem_setreg_object})

    endforeach()

    # Sets the configured values of the gem_name_template into the output argument
    list(JOIN gem_setreg_objects ",\n" gem_setreg_objects)
    string(CONFIGURE "${gems_json_template}" gem_metadata_objects @ONLY)

    # Configure the root template for the cmake_dependencies setreg file
    string(CONFIGURE "${root_dependencies_setreg_template}" root_setreg @ONLY)
    set(${output_gem_setreg_objects} ${root_setreg} PARENT_SCOPE)
endfunction()

#! ly_delayed_generate_settings_registry: Generates a .setreg file for each target with dependencies
#  added to it via ly_add_target_dependencies
#  The generated file contains the file to the each dependent targets
#  This can be used for example to determine which list of gems to load with an application
function(ly_delayed_generate_settings_registry)

    if(LY_MONOLITHIC_GAME) # No need to generate setregs for monolithic builds
        set_property(GLOBAL PROPERTY LY_DELAYED_LOAD_DEPENDENCIES) # Clear out the load targets from the global load dependencies list
        return()
    endif()

    get_property(ly_delayed_load_targets GLOBAL PROPERTY LY_DELAYED_LOAD_DEPENDENCIES)
    foreach(prefix_target_variant ${ly_delayed_load_targets})
        string(REPLACE "," ";" prefix_target_variant_list "${prefix_target_variant}")
        list(LENGTH prefix_target_variant_list prefix_target_variant_length)
        if(prefix_target_variant_length EQUAL 0)
            continue()
        endif()

        # Retrieves the prefix if available from the front of the list
        list(POP_FRONT prefix_target_variant_list prefix)
        # Retrieve the target name from the next element of the list
        list(POP_FRONT prefix_target_variant_list target)
        # Retreive the variant name from the last element of the list
        list(POP_BACK prefix_target_variant_list variant)

        # Get the gem dependencies for the given project and target combination
        get_property(target_load_dependencies GLOBAL PROPERTY LY_DELAYED_LOAD_"${prefix_target_variant}")
        message(VERBOSE "prefix=${prefix},target=${target},variant=${variant} has direct load dependencies of ${target_load_dependencies}")
        list(REMOVE_DUPLICATES target_load_dependencies) # Strip out any duplicate load dependency CMake targets
        unset(all_gem_dependencies)

        foreach(gem_target IN LISTS target_load_dependencies)
            o3de_get_gem_load_dependencies(
                GEM_LOAD_DEPENDENCIES_VAR gem_load_gem_dependencies
                TARGET "${gem_target}"
            )
            list(APPEND all_gem_dependencies ${gem_load_gem_dependencies} ${gem_target})
        endforeach()
        list(REMOVE_DUPLICATES all_gem_dependencies)

        # de-namespace them
        unset(new_gem_dependencies)
        foreach(gem_target ${all_gem_dependencies})
            ly_de_alias_target(${gem_target} stripped_gem_target)
            list(APPEND new_gem_dependencies ${stripped_gem_target})
        endforeach()
        set(all_gem_dependencies ${new_gem_dependencies})
        list(REMOVE_DUPLICATES all_gem_dependencies)

        # Fill out the gem_setreg_objects variable with the json fields for each gem
        unset(gem_setreg_objects)
        ly_populate_gem_objects(gem_load_dependencies_json "${all_gem_dependencies}")

        string(REPLACE "." "_" escaped_target ${target})
        string(JOIN "." specialization_name ${prefix} ${escaped_target})
        # Lowercase the specialization name
        string(TOLOWER ${specialization_name} specialization_name)

        get_target_property(is_imported ${target} IMPORTED)
        get_target_property(target_type ${target} TYPE)
        set(non_loadable_types "UTILITY" "INTERFACE_LIBRARY" "STATIC_LIBRARY")
        if(is_imported OR (target_type IN_LIST non_loadable_types))
            unset(target_dir)
            foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
                string(TOUPPER ${conf} UCONF)
                string(APPEND target_dir $<$<CONFIG:${conf}>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UCONF}}>)
            endforeach()
        else()
            set(target_dir $<TARGET_FILE_DIR:${target}>)
        endif()
        set(dependencies_setreg ${target_dir}/Registry/cmake_dependencies.${specialization_name}.setreg)
        file(GENERATE OUTPUT ${dependencies_setreg} CONTENT "${gem_load_dependencies_json}")
        set_property(TARGET ${target} APPEND PROPERTY INTERFACE_LY_TARGET_FILES "${dependencies_setreg}\nRegistry")

        # Clear out load dependencies for the prefix,target,variant combination
        set_property(GLOBAL PROPERTY LY_DELAYED_LOAD_"${prefix_target_variant}")
    endforeach()

    # Clear out the load targets from the global load dependencies list
    set_property(GLOBAL PROPERTY LY_DELAYED_LOAD_DEPENDENCIES)
endfunction()

