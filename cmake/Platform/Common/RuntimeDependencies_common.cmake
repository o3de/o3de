#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_COPY_PERMISSIONS "OWNER_READ OWNER_WRITE OWNER_EXECUTE")
set(LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS MODULE_LIBRARY SHARED_LIBRARY EXECUTABLE APPLICATION)

# There are several runtime dependencies to handle:
# 1. Dependencies to 3rdparty libraries. This involves copying IMPORTED_LOCATION to the folder where the target is. 
#    Some 3rdParty may require to copy the IMPORTED_LOCATION to a relative folder to where the target is.
# 2. Dependencies to files. This involves copying INTERFACE_LY_TARGET_FILES to the folder where the target is. In 
#    this case, the files may include a relative folder to where the target is.
# 3. In some platforms and types of targets, we also need to copy the MANUALLY_ADDED_DEPENDENCIES to the folder where the
#    target is. This is because the target is not in the same folder as the added dependencies. 
# In all cases, we need to recursively walk through dependencies to find the above. In multiple cases, we will end up 
# with the same files trying to be copied to the same place. This is expected, we still want to be able to produce a 
# working output per target, so there will be duplication.
#

function(ly_get_runtime_dependencies ly_RUNTIME_DEPENDENCIES ly_TARGET)
    # check to see if this target is a 3rdParty lib that was a downloaded package,
    # and if so, activate it.  This also calls find_package so there is no reason
    # to do so later.

    ly_parse_third_party_dependencies(${ly_TARGET})
    # The above needs to be done before the below early out of this function!
    if(NOT TARGET ${ly_TARGET})
        return() # Nothing to do
    endif()

    ly_de_alias_target(${ly_TARGET} ly_TARGET)

    # To optimize the search, we are going to cache the dependencies for the targets we already walked through.
    # To do so, we will create a variable named LY_RUNTIME_DEPENDENCIES_${ly_TARGET} which will contain a list
    # of all the dependencies
    # If the variable is not there, that means we have not walked it yet.
    get_property(are_dependencies_cached GLOBAL PROPERTY LY_RUNTIME_DEPENDENCIES_${ly_TARGET} SET)
    if(are_dependencies_cached)

        # We already walked through this target
        get_property(cached_dependencies GLOBAL PROPERTY LY_RUNTIME_DEPENDENCIES_${ly_TARGET})
        set(${ly_RUNTIME_DEPENDENCIES} ${cached_dependencies} PARENT_SCOPE)
        return()

    endif()

    unset(all_runtime_dependencies)
    
    # Collect all dependencies to other targets. Dependencies are through linking (LINK_LIBRARIES), and
    # other manual dependencies (MANUALLY_ADDED_DEPENDENCIES)

    get_target_property(target_type ${ly_TARGET} TYPE)
    unset(link_dependencies)
    unset(dependencies)
    get_target_property(dependencies ${ly_TARGET} INTERFACE_LINK_LIBRARIES)
    if(dependencies)
        list(APPEND link_dependencies ${dependencies})
    endif()
    if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
        unset(dependencies)
        get_target_property(dependencies ${ly_TARGET} LINK_LIBRARIES)
        if(dependencies)
            list(APPEND link_dependencies ${dependencies})
        endif()
    endif()

    # link dependencies are not runtime dependencies (we dont have anything to copy) however, we need to traverse
    # them since them or some dependency downstream could have something to copy over
    foreach(link_dependency IN LISTS link_dependencies)
        if(${link_dependency} MATCHES "^::@")
            # Skip wraping produced when targets are not created in the same directory
            # (https://cmake.org/cmake/help/latest/prop_tgt/LINK_LIBRARIES.html)
            continue()
        endif()

        if(TARGET ${link_dependency} AND link_dependency MATCHES "^3rdParty::")
            get_target_property(is_system_library ${link_dependency} LY_SYSTEM_LIBRARY)
            if(is_system_library)
                continue()
            endif()
        endif()

        unset(dependencies)
        ly_get_runtime_dependencies(dependencies ${link_dependency})
        list(APPEND all_runtime_dependencies ${dependencies})
    endforeach()

    # For manual dependencies, we want to copy over the dependency and traverse them
    unset(manual_dependencies)
    get_target_property(manual_dependencies ${ly_TARGET} MANUALLY_ADDED_DEPENDENCIES)
    if(manual_dependencies)
        foreach(manual_dependency ${manual_dependencies})
            if(NOT ${manual_dependency} MATCHES "^::@") # Skip wraping produced when targets are not created in the same directory (https://cmake.org/cmake/help/latest/prop_tgt/LINK_LIBRARIES.html)
                unset(dependencies)
                ly_get_runtime_dependencies(dependencies ${manual_dependency})
                list(APPEND all_runtime_dependencies ${dependencies})
                list(APPEND all_runtime_dependencies ${manual_dependency})
            endif()
        endforeach()
    endif()

    # Add the imported locations
    get_target_property(is_imported ${ly_TARGET} IMPORTED)
    if(is_imported)
        set(skip_imported FALSE)
        if(target_type MATCHES "(STATIC_LIBRARY)")
            # No need to copy these dependencies since the outputs are not used at runtime
            set(skip_imported TRUE)
        endif()

        if(NOT skip_imported)

            # Add imported locations
            if(target_type STREQUAL "INTERFACE_LIBRARY")
                set(imported_property INTERFACE_IMPORTED_LOCATION)
            else()
                set(imported_property IMPORTED_LOCATION)
            endif()

            unset(target_locations)
            get_target_property(target_locations ${ly_TARGET} ${imported_property})
            if(target_locations)
                list(APPEND all_runtime_dependencies "${target_locations}")
            else()
                # Check if the property exists for configurations
                unset(target_locations)
                foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
                    string(TOUPPER ${conf} UCONF)
                    unset(current_target_locations)
                    get_target_property(current_target_locations ${ly_TARGET} ${imported_property}_${UCONF})
                    if(current_target_locations)
                        string(APPEND target_locations $<$<CONFIG:${conf}>:${current_target_locations}>)
                    else()
                        # try to use the mapping
                        get_target_property(mapped_conf ${ly_TARGET} MAP_IMPORTED_CONFIG_${UCONF})
                        if(mapped_conf)
                            unset(current_target_locations)
                            get_target_property(current_target_locations ${ly_TARGET} ${imported_property}_${mapped_conf})
                            if(current_target_locations)
                                string(APPEND target_locations $<$<CONFIG:${conf}>:${current_target_locations}>)
                            endif()
                        endif()
                    endif()
                endforeach()
                if(target_locations)
                    list(APPEND all_runtime_dependencies ${target_locations})
                endif()
            endif()

        endif()

    endif()

    # Add target files (these are the ones added with ly_add_target_files)
    get_target_property(interface_target_files ${ly_TARGET} INTERFACE_LY_TARGET_FILES)
    if(interface_target_files)
        list(APPEND all_runtime_dependencies ${interface_target_files})
    endif()

    list(REMOVE_DUPLICATES all_runtime_dependencies)
    set_property(GLOBAL PROPERTY LY_RUNTIME_DEPENDENCIES_${ly_TARGET} "${all_runtime_dependencies}")
    set(${ly_RUNTIME_DEPENDENCIES} ${all_runtime_dependencies} PARENT_SCOPE)

endfunction()

function(ly_get_runtime_dependency_command ly_RUNTIME_COMMAND ly_RUNTIME_DEPEND ly_TARGET)

    # To optimize this, we are going to cache the commands for the targets we requested. A lot of targets end up being
    # dependencies of other targets. 
    get_property(is_command_cached GLOBAL PROPERTY LY_RUNTIME_DEPENDENCY_COMMAND_${ly_TARGET} SET)
    if(is_command_cached)

        # We already walked through this target
        get_property(cached_command GLOBAL PROPERTY LY_RUNTIME_DEPENDENCY_COMMAND_${ly_TARGET})
        set(${ly_RUNTIME_COMMAND} ${cached_command} PARENT_SCOPE)
        get_property(cached_depend GLOBAL PROPERTY LY_RUNTIME_DEPENDENCY_DEPEND_${ly_TARGET})
        set(${ly_RUNTIME_DEPEND} "${cached_depend}" PARENT_SCOPE)
        return()

    endif()

    unset(target_directory)
    unset(source_file)
    if(TARGET ${ly_TARGET})

        get_target_property(target_type ${ly_TARGET} TYPE)
        if(NOT target_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS)
            return()
        endif()

        set(source_file $<TARGET_FILE:${ly_TARGET}>)
        get_target_property(runtime_directory ${ly_TARGET} RUNTIME_OUTPUT_DIRECTORY)
        if(runtime_directory)
            file(RELATIVE_PATH target_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${runtime_directory})
        endif()

    else()

        string(REGEX MATCH "^([^\n]*)[\n]?(.*)$" target_file_regex "${ly_TARGET}")
        if(NOT target_file_regex)
            message(FATAL_ERROR "Unexpected error parsing \"${ly_TARGET}\"")
        endif()
        set(source_file ${CMAKE_MATCH_1})
        set(target_directory ${CMAKE_MATCH_2})

    endif()
    if(target_directory)
        string(PREPEND target_directory /)
    endif()

    # Some notes on the generated command:
    # To support platforms where the binaries end in different places, we are going to assume that all dependencies,
    # including the ones we are building, need to be copied over. However, we add a check to prevent copying something
    # over itself. This detection cannot happen now because the target we are copying for varies.
    set(runtime_command "ly_copy(\"${source_file}\" \"@target_file_dir@${target_directory}\")\n")

    # Tentative optimization: this is an attempt to solve the first "if" at generation time, making the runtime_dependencies
    # file smaller and faster to run. In platforms where the built target and the dependencies targets end up in the same
    # place, we end up with a lot of dependencies that dont need to copy anything.
    # However, the generation expression ends up being complicated and the parser seems to get really confused. My hunch
    # is that the string we are pasting has generation expressions, and those commas confuse the "if" generator expression.
    # Leaving the attempt here commented out until I can get back to it.
#    set(generated_command "
#if(NOT EXISTS \"$<TARGET_FILE_DIR:@target@>${target_directory}\")
#    file(MAKE_DIRECTORY \"$<TARGET_FILE_DIR:@target@>${target_directory}\")
#endif()
#if(\"${source_file}\" IS_NEWER_THAN \"$<TARGET_FILE_DIR:@target@>${target_directory}/${target_filename}\")
#    file(COPY \"${source_file}\" DESTINATION \"$<TARGET_FILE_DIR:@target@>${target_directory}\" FILE_PERMISSIONS ${LY_COPY_PERMISSIONS})
#endif()
#")
#    set(runtime_command "$<IF:$<STREQUAL:$<GENEX_EVAL:\"${source_file}\">,$<GENEX_EVAL:\"$<TARGET_FILE_DIR:@target@>${target_directory}/${target_filename}>\">>,\"\",\"$<GENEX_EVAL:${generated_command}>\">")

    set_property(GLOBAL PROPERTY LY_RUNTIME_DEPENDENCY_COMMAND_${ly_TARGET} "${runtime_command}")
    set(${ly_RUNTIME_COMMAND} ${runtime_command} PARENT_SCOPE)
    set_property(GLOBAL PROPERTY LY_RUNTIME_DEPENDENCY_DEPEND_${ly_TARGET} "${source_file}")
    set(${ly_RUNTIME_DEPEND} "${source_file}" PARENT_SCOPE)

endfunction()

function(ly_delayed_generate_runtime_dependencies)

    get_property(additional_module_paths GLOBAL PROPERTY LY_ADDITIONAL_MODULE_PATH)
    list(APPEND CMAKE_MODULE_PATH ${additional_module_paths})

    get_property(all_targets GLOBAL PROPERTY LY_ALL_TARGETS)
    foreach(aliased_target IN LISTS all_targets)

        unset(target)
        ly_de_alias_target(${aliased_target} target)

        # Exclude targets that dont produce runtime outputs
        get_target_property(target_type ${target} TYPE)
        if(NOT target_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS)
            continue()
        endif()

        unset(runtime_dependencies)
        unset(LY_COPY_COMMANDS)
        unset(runtime_depends)

        ly_get_runtime_dependencies(runtime_dependencies ${target})
        foreach(runtime_dependency ${runtime_dependencies})
            unset(runtime_command)
            unset(runtime_depend)
            ly_get_runtime_dependency_command(runtime_command runtime_depend ${runtime_dependency})
            string(APPEND LY_COPY_COMMANDS ${runtime_command})
            list(APPEND runtime_depends ${runtime_depend})
        endforeach()

        # Generate the output file, note the STAMP_OUTPUT_FILE need to match with the one defined in LYWrappers.cmake
        set(STAMP_OUTPUT_FILE ${CMAKE_BINARY_DIR}/runtime_dependencies/$<CONFIG>/${target}_$<CONFIG>.stamp)
        set(target_file_dir "$<TARGET_FILE_DIR:${target}>")
        set(target_file "$<TARGET_FILE:${target}>")
        ly_file_read(${LY_RUNTIME_DEPENDENCIES_TEMPLATE} template_file)
        string(CONFIGURE "${LY_COPY_COMMANDS}" LY_COPY_COMMANDS @ONLY)
        string(CONFIGURE "${template_file}" configured_template_file @ONLY)
        file(GENERATE
            OUTPUT ${CMAKE_BINARY_DIR}/runtime_dependencies/$<CONFIG>/${target}.cmake
            CONTENT "${configured_template_file}"
        )
        
        # set the property that is consumed from the custom command generated in LyWrappers.cmake
        set_target_properties(${target} PROPERTIES RUNTIME_DEPENDENCIES_DEPENDS "${runtime_depends}")

    endforeach()

endfunction()
