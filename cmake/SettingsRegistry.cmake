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
}]]
)
set(gem_module_template [[
            "@stripped_gem_target@":
            {
                "Modules":["$<TARGET_FILE_NAME:@gem_target@>"],
                "SourcePaths":["@gem_relative_source_dir@"]
            }]]
)

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

    # Optimize the search by caching gem load dependencies
    get_property(are_dependencies_cached GLOBAL PROPERTY LY_GEM_LOAD_DEPENDENCIES_${ly_TARGET} SET)
    if(are_dependencies_cached)
        # We already walked through this target
        get_property(cached_dependencies GLOBAL PROPERTY LY_GEM_LOAD_DEPENDENCIES_${ly_TARGET})
        set(${ly_GEM_LOAD_DEPENDENCIES} ${cached_dependencies} PARENT_SCOPE)
        return()
    endif()

    unset(all_gem_load_dependencies)

    # For load dependencies, we want to copy over the dependency and traverse them
    # Only if the dependency has a GEM_MODULE property
    unset(load_dependencies)
    get_target_property(load_dependencies ${ly_TARGET} MANUALLY_ADDED_DEPENDENCIES)
    if(load_dependencies)
        foreach(load_dependency ${load_dependencies})
            # Skip wrapping produced when targets are not created in the same directory 
            if(NOT ${load_dependency} MATCHES "^::@")
                get_property(dependency_type TARGET ${load_dependency} PROPERTY TYPE)
                get_property(is_gem_target TARGET ${load_dependency} PROPERTY GEM_MODULE SET)
                # If the dependency is a "gem module" then add it as a load dependencies
                # and recurse into its manually added dependencies
                if (is_gem_target)
                    unset(dependencies)
                    ly_get_gem_load_dependencies(dependencies ${load_dependency})
                    list(APPEND all_gem_load_dependencies ${load_dependency})
                    list(APPEND all_gem_load_dependencies ${dependencies})
                endif()
            endif()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES all_gem_load_dependencies)
    set_property(GLOBAL PROPERTY LY_GEM_LOAD_DEPENDENCIES_${ly_TARGET} "${all_gem_load_dependencies}")
    set(${ly_GEM_LOAD_DEPENDENCIES} ${all_gem_load_dependencies} PARENT_SCOPE)
endfunction()

#! ly_delayed_generate_settings_registry: Generates a .setreg file for each target with dependencies
#  added to it via ly_add_target_dependencies
#  The generated file contains the file to the each dependent targets
#  This can be used for example to determine which list of gems to load with an application
function(ly_delayed_generate_settings_registry)
    get_property(ly_delayed_load_targets GLOBAL PROPERTY LY_DELAYED_LOAD_DEPENDENCIES)

    foreach(prefix_target ${ly_delayed_load_targets})
        string(REPLACE "," ";" prefix_target_list "${prefix_target}")
        list(LENGTH prefix_target_list prefix_target_length)
        if(prefix_target_length EQUAL 0)
            message(SEND_ERROR "Delayed load target is missing target name")
            continue()
        endif()

        # Retrieve the target name from the back of the list
        list(POP_BACK prefix_target_list target)
        # Retreives the prefix if available from the remaining element of the list
        list(POP_BACK prefix_target_list prefix)

        # Get the gem dependencies for the given project and target combination
        get_property(gem_dependencies GLOBAL PROPERTY LY_DELAYED_LOAD_"${prefix_target}")
        list(REMOVE_DUPLICATES gem_dependencies) # Strip out any duplicate gem targets
        set(all_gem_dependencies ${gem_dependencies})

        foreach(gem_target ${gem_dependencies})
            ly_get_gem_load_dependencies(gem_load_gem_dependencies ${gem_target})
            list(APPEND all_gem_dependencies ${gem_load_gem_dependencies})
        endforeach()
        list(REMOVE_DUPLICATES all_gem_dependencies)

        unset(target_gem_dependencies_names)
        foreach(gem_target ${all_gem_dependencies})
            unset(gem_relative_source_dir)
            # Create path from the LY_ROOT_FOLDER to the the Gem directory
            if (NOT TARGET ${gem_target})
                message(FATAL_ERROR "Dependency ${gem_target} from ${target} does not exist")
            endif()
            get_property(gem_relative_source_dir TARGET ${gem_target} PROPERTY SOURCE_DIR)
            if(gem_relative_source_dir)
                # Most gems SOURCE dir is nested in the path, we need to find the path to the gem.json or project.json file
                while(NOT EXISTS ${gem_relative_source_dir}/gem.json AND NOT EXISTS ${gem_relative_source_dir}/project.json)
                    get_filename_component(parent_dir ${gem_relative_source_dir} DIRECTORY)
                    if (${parent_dir} STREQUAL ${gem_relative_source_dir})
                        message(FATAL_ERROR "Did not find gem.json or project.json while processing target ${gem_target}!")
                    endif()
                    set(gem_relative_source_dir ${parent_dir})
                endwhile()
                file(TO_CMAKE_PATH ${LY_ROOT_FOLDER} ly_root_folder_cmake)
                file(RELATIVE_PATH gem_relative_source_dir ${ly_root_folder_cmake} ${gem_relative_source_dir})
            endif()

            # Strip target namespace from gem targets before configuring them into the json template
            ly_strip_target_namespace(TARGET ${gem_target} OUTPUT_VARIABLE stripped_gem_target)
            string(CONFIGURE ${gem_module_template} gem_module_json @ONLY)
            list(APPEND target_gem_dependencies_names ${gem_module_json})
        endforeach()

        string(REPLACE "." "_" escaped_target ${target})
        string(JOIN "." specialization_name ${prefix} ${escaped_target})
        # Lowercase the specialization name
        string(TOLOWER ${specialization_name} specialization_name)

        list(JOIN target_gem_dependencies_names ",\n" target_gem_dependencies_names)
        string(CONFIGURE ${gems_json_template} gem_json @ONLY)
        if(prefix)
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

    # Clear out the load targets from the glboal load dependencies list
    set_property(GLOBAL PROPERTY LY_DELAYED_LOAD_DEPENDENCIES)
endfunction()

