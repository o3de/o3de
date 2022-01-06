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

#templates used to generate the gems json
set(gems_json_template [[
{
    "Amazon":
    {
        "Gems":
        {
@target_gem_dependencies_names@
        }
    }
}
]]
)
 string(APPEND gem_module_template
[=[            "@stripped_gem_target@":]=] "\n"
[=[            {]=] "\n"
[=[$<$<IN_LIST:$<TARGET_PROPERTY:@gem_target@,TYPE>,MODULE_LIBRARY$<SEMICOLON>SHARED_LIBRARY>:                "Modules":["$<TARGET_FILE_NAME:@gem_target@>"]]=] "$<COMMA>\n>"
[=[                "SourcePaths":["@gem_module_root_relative_to_engine_root@"]]=] "\n"
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

#!ly_get_gem_load_dependencies: Retrieves the list of "load" dependencies for a target
# Visits through only MANUALLY_ADDED_DEPENDENCIES of targets with a GEM_MODULE property
# to determine which gems a target needs to load
# ly_get_runtime_dependencies cannot be used as it will recurse through non-manually added dependencies
# to add manually added added which results in false load dependencies.
# \arg:ly_GEM_LOAD_DEPENDENCIES(name) - Output variable to be populated gem load dependencies
# \arg:ly_TARGET(TARGET) - CMake target to examine for dependencies
function(ly_get_gem_load_dependencies ly_GEM_LOAD_DEPENDENCIES ly_TARGET)
    if(NOT TARGET ${ly_TARGET})
        return() # Nothing to do
    endif()
    # Internally we use a third parameter to pass the list of targets that we have traversed. This is 
    # used to detect runtime cycles
    if(ARGC EQUAL 3)
        set(ly_CYCLE_DETECTION_TARGETS ${ARGV2})
    else()
        set(ly_CYCLE_DETECTION_TARGETS "")
    endif()

    # Optimize the search by caching gem load dependencies
    get_property(are_dependencies_cached GLOBAL PROPERTY LY_GEM_LOAD_DEPENDENCIES_${ly_TARGET} SET)
    if(are_dependencies_cached)
        # We already walked through this target
        get_property(cached_dependencies GLOBAL PROPERTY LY_GEM_LOAD_DEPENDENCIES_${ly_TARGET})
        set(${ly_GEM_LOAD_DEPENDENCIES} ${cached_dependencies} PARENT_SCOPE)
        return()
    endif()

    # detect cycles
    unset(cycle_detected)
    ly_detect_cycle_through_visitation(${ly_TARGET} "${ly_CYCLE_DETECTION_TARGETS}" ly_CYCLE_DETECTION_TARGETS cycle_detected)
    if(cycle_detected)
        message(FATAL_ERROR "Runtime dependency detected: ${cycle_detected}")
    endif()

    unset(all_gem_load_dependencies)

    # For load dependencies, we want to copy over the dependency and traverse them
    # Only if the dependency has a GEM_MODULE property
    unset(load_dependencies)
    get_target_property(load_dependencies ${ly_TARGET} MANUALLY_ADDED_DEPENDENCIES)
    if(load_dependencies)
        foreach(load_dependency ${load_dependencies})
            # Skip wrapping produced when targets are not created in the same directory
            ly_de_alias_target(${load_dependency} dealias_load_dependency)
            get_property(is_gem_target TARGET ${dealias_load_dependency} PROPERTY GEM_MODULE SET)
            # If the dependency is a "gem module" then add it as a load dependencies
            # and recurse into its manually added dependencies
            if (is_gem_target)
                unset(dependencies)
                ly_get_gem_load_dependencies(dependencies ${dealias_load_dependency} "${ly_CYCLE_DETECTION_TARGETS}")
                list(APPEND all_gem_load_dependencies ${dependencies})
                list(APPEND all_gem_load_dependencies ${dealias_load_dependency})
            endif()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES all_gem_load_dependencies)
    set_property(GLOBAL PROPERTY LY_GEM_LOAD_DEPENDENCIES_${ly_TARGET} "${all_gem_load_dependencies}")
    set(${ly_GEM_LOAD_DEPENDENCIES} ${all_gem_load_dependencies} PARENT_SCOPE)
    message(VERBOSE "Gem Target \"${ly_TARGET}\" has load dependencies of: ${all_gem_load_dependencies}")

endfunction()

#!ly_get_gem_module_root: Uses the supplied gem_target to lookup the nearest gem.json file above the SOURCE_DIR
#
# \arg:gem_target(TARGET) - Target to look upwards from using its SOURCE_DIR property
function(ly_get_gem_module_root output_gem_module_root gem_target)
    unset(${output_gem_module_root} PARENT_SCOPE)
    get_property(gem_source_dir TARGET ${gem_target} PROPERTY SOURCE_DIR)

    if(gem_source_dir)
        set(candidate_gem_dir ${gem_source_dir})
        # Locate the root of the gem by finding the gem.json location
        while(NOT EXISTS ${candidate_gem_dir}/gem.json)
            get_filename_component(parent_dir ${candidate_gem_dir} DIRECTORY)
            if (${parent_dir} STREQUAL ${candidate_gem_dir})
                # "Did not find a gem.json while processing GEM_MODULE target ${gem_target}!"
                return()
            endif()
            set(candidate_gem_dir ${parent_dir})
        endwhile()
    endif()

    if (EXISTS ${candidate_gem_dir}/gem.json)
        set(gem_source_dir ${candidate_gem_dir})
    endif()

    # Set the gem module root output directory to the location with the gem.json file within it or
    # the supplied gem_target SOURCE_DIR location if no gem.json file was found
    set(${output_gem_module_root} ${gem_source_dir} PARENT_SCOPE)
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
    foreach(prefix_target ${ly_delayed_load_targets})
        string(REPLACE "," ";" prefix_target_list "${prefix_target}")
        list(LENGTH prefix_target_list prefix_target_length)
        if(prefix_target_length EQUAL 0)
            continue()
        endif()

        # Retrieve the target name from the back of the list
        list(POP_BACK prefix_target_list target)
        # Retrieves the prefix if available from the remaining element of the list
        list(POP_BACK prefix_target_list prefix)

        # Get the gem dependencies for the given project and target combination
        get_property(gem_dependencies GLOBAL PROPERTY LY_DELAYED_LOAD_"${prefix_target}")
        list(REMOVE_DUPLICATES gem_dependencies) # Strip out any duplicate gem targets
        unset(all_gem_dependencies)

        foreach(gem_target ${gem_dependencies})
            ly_get_gem_load_dependencies(gem_load_gem_dependencies ${gem_target})
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

        unset(target_gem_dependencies_names)
        foreach(gem_target ${all_gem_dependencies})
            unset(gem_relative_source_dir)
            # Create path from the LY_ROOT_FOLDER to the the Gem directory
            if (NOT TARGET ${gem_target})
                message(FATAL_ERROR "Dependency ${gem_target} from ${target} does not exist")
            endif()

            ly_get_gem_module_root(gem_module_root ${gem_target})
            if (NOT gem_module_root)
                # If the target doesn't have a gem.json, skip it
                continue()
            endif()
            file(RELATIVE_PATH gem_module_root_relative_to_engine_root ${LY_ROOT_FOLDER} ${gem_module_root})

            # De-alias namespace from gem targets before configuring them into the json template
            ly_de_alias_target(${gem_target} stripped_gem_target)
            string(CONFIGURE "${gem_module_template}" gem_module_json @ONLY)
            list(APPEND target_gem_dependencies_names ${gem_module_json})
        endforeach()

        string(REPLACE "." "_" escaped_target ${target})
        string(JOIN "." specialization_name ${prefix} ${escaped_target})
        # Lowercase the specialization name
        string(TOLOWER ${specialization_name} specialization_name)

        list(JOIN target_gem_dependencies_names ",\n" target_gem_dependencies_names)
        string(CONFIGURE ${gems_json_template} gem_json @ONLY)
        get_target_property(is_imported ${target} IMPORTED)
        get_target_property(target_type ${target} TYPE)
        set(non_loadable_types "UTILITY" "INTERFACE_LIBRARY" "STATIC_LIBRARY")
        if(is_imported OR (target_type IN_LIST non_loadable_types))
            unset(target_dir)
            foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
                string(TOUPPER ${conf} UCONF)
                string(APPEND target_dir $<$<CONFIG:${conf}>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UCONF}}>)
            endforeach()
        elseif(prefix)
            set(target_dir $<TARGET_FILE_DIR:${prefix}>)
        else()
            set(target_dir $<TARGET_FILE_DIR:${target}>)
        endif()
        set(dependencies_setreg ${target_dir}/Registry/cmake_dependencies.${specialization_name}.setreg)
        file(GENERATE OUTPUT ${dependencies_setreg} CONTENT ${gem_json})
        set_property(TARGET ${target} APPEND PROPERTY INTERFACE_LY_TARGET_FILES "${dependencies_setreg}\nRegistry")

        # Clear out load dependencies for the project/target combination
        set_property(GLOBAL PROPERTY LY_DELAYED_LOAD_"${prefix_target}")
    endforeach()

    # Clear out the load targets from the global load dependencies list
    set_property(GLOBAL PROPERTY LY_DELAYED_LOAD_DEPENDENCIES)
endfunction()

