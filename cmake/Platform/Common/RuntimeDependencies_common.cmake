#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_COPY_PERMISSIONS "OWNER_READ OWNER_WRITE OWNER_EXECUTE")
set(LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS UTILITY MODULE_LIBRARY SHARED_LIBRARY EXECUTABLE APPLICATION)

# There are several dependencies to handle:
# 1. Dependencies to 3rdparty libraries. This involves copying IMPORTED_LOCATION to the folder where the target is.
#    Some 3rdParty may require to copy the IMPORTED_LOCATION to a relative folder to where the target is.
# 2. Dependencies to files. This involves copying INTERFACE_LY_TARGET_FILES to the folder where the target is. In
#    this case, the files may include a relative folder to where the target is.
# 3. In some platforms and types of targets, we also need to copy the MANUALLY_ADDED_DEPENDENCIES to the folder where the
#    target is. This is because the target is not in the same folder as the added dependencies.
# In all cases, we need to recursively walk through dependencies to find the above. In multiple cases, we will end up
# with the same files trying to be copied to the same place. This is expected, we still want to be able to produce a
# working output per target, so there will be duplication.
# The Dependencies get stored in the following args:
# \arg: COPY_DEPENDENCIES_VAR stores the encoded file dependency with the output subdirectory to copy the file dependency
#        These dependencies are set via the `ly_add_target_files` command
# \arg: TARGET_DEPENDENCIES_VAR stores all MANUALLY_ADDED_DEPENDENCIES for the target by recursively visiting
#       each target added dependencies
#       These dependences are set via [add_dependencies](https://cmake.org/cmake/help/latest/command/add_dependencies.html)
# \arg: LINK_DEPENDENCIES_VAR stores all link library dependencies found by recursing the LINK_LIBRARIES TARGET property
#       These are dependencies specified to the [target_link_libraries](https://cmake.org/cmake/help/latest/command/target_link_libraries.html?highlight=target_link_libraries) command
# \arg: IMPORTED_DEPENDENCIES_VAR populated with the IMPORTED_LOCATION(_<config>) or INTERFACE_IMPORTED_LOCATION(_<config>) property of the TARGET
#       when the target is an IMPORTED library with a pre-built binary
# Regular Arguments are as follows:
# \arg: TARGET CMake target to search for the dependencies
function(o3de_get_dependencies_for_target)
    set(options)
    set(oneValueArgs TARGET COPY_DEPENDENCIES_VAR TARGET_DEPENDENCIES_VAR LINK_DEPENDENCIES_VAR IMPORTED_DEPENDENCIES_VAR)
    set(multiValueArgs)
    cmake_parse_arguments("${CMAKE_CURRENT_FUNCTION}" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(target "${${CMAKE_CURRENT_FUNCTION}_TARGET}")
    set(copy_dependencies_var "${${CMAKE_CURRENT_FUNCTION}_COPY_DEPENDENCIES_VAR}")
    set(target_dependencies_var "${${CMAKE_CURRENT_FUNCTION}_TARGET_DEPENDENCIES_VAR}")
    set(link_dependencies_var "${${CMAKE_CURRENT_FUNCTION}_LINK_DEPENDENCIES_VAR}")
    set(imported_dependencies_var "${${CMAKE_CURRENT_FUNCTION}_IMPORTED_DEPENDENCIES_VAR}")

    # check to see if this target is a 3rdParty lib that was a downloaded package,
    # and if so, activate it.  This also calls find_package so there is no reason
    # to do so later.

    ly_parse_third_party_dependencies(${target})
    # The above needs to be done before the below early out of this function!
    if(NOT TARGET ${target})
        return() # Nothing to do
    endif()

    ly_de_alias_target(${target} target)

    # To optimize the search, we are going to cache the dependencies for the targets we already walked through.
    # To do so, we will create several variables named target} which will contain a list
    # of all the dependencies
    # If the variable is not there, that means we have not walked it yet.
    get_property(are_dependencies_cached GLOBAL PROPERTY O3DE_DEPENDENCIES_CACHED_${target} SET)
    if(are_dependencies_cached)
        # We already walked through this target
        if (NOT copy_dependencies_var STREQUAL "")
            get_property(copy_dependencies GLOBAL PROPERTY O3DE_COPY_DEPENDENCIES_${target})
            set(${copy_dependencies_var} ${copy_dependencies} PARENT_SCOPE)
        endif()
        if (NOT target_dependencies_var STREQUAL "")
            get_property(target_dependencies GLOBAL PROPERTY O3DE_TARGET_DEPENDENCIES_${target})
            set(${target_dependencies_var} ${target_dependencies} PARENT_SCOPE)
        endif()
        if (NOT link_dependencies_var STREQUAL "")
            get_property(link_dependencies GLOBAL PROPERTY O3DE_LINK_DEPENDENCIES_${target})
            set(${link_dependencies_var} ${link_dependencies} PARENT_SCOPE)
        endif()
        if (NOT imported_dependencies_var STREQUAL "")
            get_property(imported_dependencies GLOBAL PROPERTY O3DE_IMPORTED_DEPENDENCIES_${target})
            set(${imported_dependencies_var} ${imported_dependencies} PARENT_SCOPE)
        endif()
        return()

    endif()

    # Unset the aggregate dependency list variables
    unset(all_copy_dependencies)
    unset(all_target_dependencies)
    unset(all_link_dependencies)
    unset(all_imported_dependencies)

    # Collect all dependencies to other targets. Dependencies are through linking (LINK_LIBRARIES), and
    # other manual dependencies (MANUALLY_ADDED_DEPENDENCIES)

    get_target_property(target_type ${target} TYPE)
    unset(link_dependencies)
    unset(dependencies)
    get_target_property(dependencies ${target} INTERFACE_LINK_LIBRARIES)
    if(dependencies)
        list(APPEND link_dependencies ${dependencies})
    endif()
    if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
        unset(dependencies)
        get_target_property(dependencies ${target} LINK_LIBRARIES)
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

        if(TARGET ${link_dependency})
            get_target_property(is_imported ${link_dependency} IMPORTED)
            get_target_property(is_system_library ${link_dependency} LY_SYSTEM_LIBRARY)
            if(is_imported AND is_system_library)
                continue()
            endif()

            # If the link dependency target has runtime outputs itself then
            # add it as a runtime dependency as well.
            get_target_property(link_dependency_type ${link_dependency} TYPE)
            if(link_dependency_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS)
                list(APPEND all_link_dependencies ${link_dependency})
            endif()
        endif()

        unset(dependent_copy_dependencies)
        unset(dependent_target_dependencies)
        unset(dependent_link_dependencies)
        unset(dependent_imported_dependencies)
        o3de_get_dependencies_for_target(
            TARGET "${link_dependency}"
            COPY_DEPENDENCIES_VAR dependent_copy_dependencies
            TARGET_DEPENDENCIES_VAR dependent_target_dependencies
            LINK_DEPENDENCIES_VAR dependent_link_dependencies
            IMPORTED_DEPENDENCIES_VAR dependent_imported_dependencies
        )
        list(APPEND all_copy_dependencies ${dependent_copy_dependencies})
        list(APPEND all_target_dependencies ${dependent_target_dependencies})
        list(APPEND all_link_dependencies ${dependent_link_dependencies})
        list(APPEND all_imported_dependencies ${dependent_imported_dependencies})
    endforeach()


    # For manual dependencies, we want to copy over the dependency and traverse them\
    unset(manual_dependencies)
    get_target_property(manual_dependencies ${target} MANUALLY_ADDED_DEPENDENCIES)
    if(manual_dependencies)
        foreach(manual_dependency ${manual_dependencies})
            if(NOT ${manual_dependency} MATCHES "^::@") # Skip wraping produced when targets are not created in the same directory (https://cmake.org/cmake/help/latest/prop_tgt/LINK_LIBRARIES.html)
                unset(dependent_copy_dependencies)
                unset(dependent_target_dependencies)
                unset(dependent_link_dependencies)
                unset(dependent_imported_dependencies)
                o3de_get_dependencies_for_target(
                    TARGET "${manual_dependency}"
                    COPY_DEPENDENCIES_VAR dependent_copy_dependencies
                    TARGET_DEPENDENCIES_VAR dependent_target_dependencies
                    LINK_DEPENDENCIES_VAR dependent_link_dependencies
                    IMPORTED_DEPENDENCIES_VAR dependent_imported_dependencies
                )
                list(APPEND all_copy_dependencies ${dependent_copy_dependencies})
                list(APPEND all_target_dependencies ${dependent_target_dependencies})
                list(APPEND all_link_dependencies ${dependent_link_dependencies})
                list(APPEND all_imported_dependencies ${dependent_imported_dependencies})

                # Append the current manuallly added dependency to the end
                list(APPEND all_target_dependencies ${manual_dependency})
            endif()
        endforeach()
    endif()

    # Add the imported locations
    get_target_property(is_imported ${target} IMPORTED)
    if(is_imported)
        set(skip_imported FALSE)
        if(target_type MATCHES "(STATIC_LIBRARY|OBJECT_LIBRARY)")
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

            # The below loop emulates CMake's search for imported locations:
            unset(target_locations)
            foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
                string(TOUPPER ${conf} UCONF)
                # try to use the mapping
                get_target_property(mapped_conf ${target} MAP_IMPORTED_CONFIG_${UCONF})
                if(NOT mapped_conf)
                    # if there's no mapping specified, prefer a matching conf if its available
                    # and if its not, fall back to the blank/empty one (the semicolon is not a typo)
                    set(mapped_conf "${UCONF};")
                endif()

                unset(current_target_locations)
                # note that mapped_conf is a LIST, we need to iterate the list and find the first one that exists:
                foreach(check_conf IN LISTS mapped_conf)
                    # check_conf will either be a string like "RELEASE" or the blank empty string.
                    if (check_conf)
                        # a non-empty element indicates to look fro IMPORTED_LOCATION_xxxxxxxxx
                        get_target_property(current_target_locations ${target} ${imported_property}_${check_conf})
                    else()
                        # an empty element indicates to look at the IMPORTED_LOCATION with no suffix.
                        get_target_property(current_target_locations ${target} ${imported_property})
                    endif()

                    if(current_target_locations)
                        # we need to escape any semicolons, since this could be a list.
                        string(REPLACE ";" "$<SEMICOLON>" current_target_locations "${current_target_locations}")
                        string(APPEND target_locations "$<$<CONFIG:${conf}>:${current_target_locations}>")
                        break() # stop looking after the first one is found (This emulates CMakes behavior)
                    endif()
                endforeach()
                if (NOT current_target_locations)
                    # we didn't find any locations.
                    if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
                        # If you explicitly chose to declare a STATIC library but not supply an imported location
                        # then its a mistake.
                        message(FATAL_ERROR "${target_type} Library ${target} specified MAP_IMPORTED_CONFIG_${UCONF} = ${mapped_conf} but did not have any of ${imported_property}_xxxx set")
                    endif()
                endif()
            endforeach()
            if(target_locations)
                list(APPEND all_imported_dependencies ${target_locations})
            endif()

        endif()

    endif()

    # Add target files (these are the ones added with ly_add_target_files)
    get_target_property(interface_target_files ${target} INTERFACE_LY_TARGET_FILES)
    if(interface_target_files)
        list(APPEND all_copy_dependencies ${interface_target_files})
    endif()

    # Remove duplicates from each set of dependencies
    list(REMOVE_DUPLICATES all_copy_dependencies)
    list(REMOVE_DUPLICATES all_target_dependencies)
    list(REMOVE_DUPLICATES all_link_dependencies)
    list(REMOVE_DUPLICATES all_imported_dependencies)
    # Add each type of dependencies to global property which act as a cache for
    # future calls with the same target
    set_property(GLOBAL PROPERTY O3DE_COPY_DEPENDENCIES_${target} ${all_copy_dependencies})
    set_property(GLOBAL PROPERTY O3DE_TARGET_DEPENDENCIES_${target} ${all_target_dependencies})
    set_property(GLOBAL PROPERTY O3DE_LINK_DEPENDENCIES_${target} ${all_link_dependencies})
    set_property(GLOBAL PROPERTY O3DE_IMPORTED_DEPENDENCIES_${target} ${all_imported_dependencies})
    # Set the O3DE_DEPENDENCIES_CACHED property for the target to indicate
    # the dependency search does not need to be done again
    set_property(GLOBAL PROPERTY O3DE_DEPENDENCIES_CACHED_${target} TRUE)
    # Update each of result variables for each typer of dependencies
    if (NOT copy_dependencies_var STREQUAL "")
        set(${copy_dependencies_var} ${all_copy_dependencies} PARENT_SCOPE)
    endif()
    if (NOT target_dependencies_var STREQUAL "")
        set(${target_dependencies_var} ${all_target_dependencies} PARENT_SCOPE)
    endif()
    if (NOT link_dependencies_var STREQUAL "")
        set(${link_dependencies_var} ${all_link_dependencies} PARENT_SCOPE)
    endif()
    if (NOT imported_dependencies_var STREQUAL "")
        set(${imported_dependencies_var} ${all_imported_dependencies} PARENT_SCOPE)
    endif()
endfunction()

#! This function accepts a dependency and returns
#! a command to perform on that dependency as well as a file dependency
#! which can be provided to the cmake to trigger the command to re-run
# If the result variable is not supplied, nothing is set
function(o3de_get_command_for_dependency)
    set(options)
    set(oneValueArgs DEPENDENCY COMMAND_VAR FILE_DEPENDENCY_VAR)
    set(multiValueArgs)
    cmake_parse_arguments("${CMAKE_CURRENT_FUNCTION}" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(dependency "${${CMAKE_CURRENT_FUNCTION}_DEPENDENCY}")
    set(command_var "${${CMAKE_CURRENT_FUNCTION}_COMMAND_VAR}")
    set(file_dependency_var "${${CMAKE_CURRENT_FUNCTION}_FILE_DEPENDENCY_VAR}")

    # To optimize this, we are going to cache the commands for the targets we requested. A lot of targets end up being
    # dependencies of other targets.
    get_property(is_command_cached GLOBAL PROPERTY O3DE_COMMAND_FOR_DEPENDENCY_${dependency} SET)
    if(is_command_cached)
        # We already walked through this target
        if (NOT command_var STREQUAL "")
            get_property(cached_command GLOBAL PROPERTY O3DE_COMMAND_FOR_DEPENDENCY_${dependency})
            set(${command_var} ${cached_command} PARENT_SCOPE)
        endif()
        if (NOT file_dependency_var STREQUAL "")
            get_property(cached_depend GLOBAL PROPERTY O3DE_FILE_DEPENDENCY_FOR_DEPENDENCY_${dependency})
            set(${file_dependency_var} "${cached_depend}" PARENT_SCOPE)
        endif()
        return()
    endif()

    unset(target_directory)
    unset(source_file)
    if(TARGET ${dependency})

        get_target_property(target_type ${dependency} TYPE)
        if(NOT target_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS OR target_type STREQUAL UTILITY)
            return()
        endif()

        set(source_file $<TARGET_FILE:${dependency}>)
        get_target_property(runtime_directory ${dependency} RUNTIME_OUTPUT_DIRECTORY)
        if(runtime_directory)
            file(RELATIVE_PATH target_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${runtime_directory})
        endif()

        # Query the SOURCE TYPE from the Target and pass it to the ly_copy command below
        get_property(source_type TARGET ${dependency} PROPERTY TYPE)
        # Also query if the source target is a gem module as well
        get_property(source_gem_module TARGET ${dependency} PROPERTY GEM_MODULE)

    else()

        string(REGEX MATCH "^([^\n]*)[\n]?(.*)$" target_file_regex "${dependency}")
        if(NOT target_file_regex)
            message(FATAL_ERROR "Unexpected error parsing \"${dependency}\"")
        endif()
        set(source_file ${CMAKE_MATCH_1})
        set(target_directory ${CMAKE_MATCH_2})

    endif()

    # Some notes on the generated command:
    # To support platforms where the binaries end in different places, we are going to assume that all dependencies,
    # including the ones we are building, need to be copied over. However, we add a check to prevent copying something
    # over itself. This detection cannot happen now because the target we are copying for varies.
    unset(runtime_command)
    string(APPEND runtime_command "ly_copy(\"${source_file}\" \"${target_directory}\" TARGET_FILE_DIR \"@target_file_dir@\""
        " SOURCE_TYPE \"${source_type}\" SOURCE_GEM_MODULE \"${source_gem_module}\")\n")

    set_property(GLOBAL PROPERTY O3DE_COMMAND_FOR_DEPENDENCY_${dependency} "${runtime_command}")
    set(${command_var} ${runtime_command} PARENT_SCOPE)
    if(NOT command_var STREQUAL "")
        set_property(GLOBAL PROPERTY O3DE_FILE_DEPENDENCY_FOR_DEPENDENCY_${dependency} "${source_file}")
    endif()
    if (NOT file_dependency_var STREQUAL "")
        set(${file_dependency_var} "${source_file}" PARENT_SCOPE)
    endif()

endfunction()

#! Extracts the source file tied to the dependency
#! The dependency can be a TARGET or a file
function(o3de_get_file_from_dependency)
    set(options)
    set(oneValueArgs SOURCE_FILE_VAR DEPENDENCY)
    set(multiValueArgs)
    cmake_parse_arguments("${CMAKE_CURRENT_FUNCTION}" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(dependency "${${CMAKE_CURRENT_FUNCTION}_DEPENDENCY}")
    set(source_file_var "${${CMAKE_CURRENT_FUNCTION}_SOURCE_FILE_VAR}")
    # Clear the variable pointed to by the source_file_var in the PARENT_SCOPE
    unset("${source_file_var}" PARENT_SCOPE)

    unset(source_file)
    if(TARGET ${dependency})
        get_property(target_type TARGET ${dependency} PROPERTY TYPE)
        if(NOT target_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS OR target_type STREQUAL UTILITY)
            return()
        endif()
        set(source_file $<TARGET_FILE:${dependency}>)
    else()
        string(REGEX MATCH "^([^\n]*)[\n]?(.*)$" file_regex "${dependency}")
        if(NOT file_regex)
            message(FATAL_ERROR "Cannot parse source file from \"${dependency}\" using regular expression")
        endif()
        cmake_path(SET source_file NORMALIZE ${CMAKE_MATCH_1})
    endif()

    if (source_file)
        set("${source_file_var}" "${source_file}" PARENT_SCOPE)
    endif()
endfunction()

function(o3de_transform_dependencies_to_files)
    set(options)
    set(oneValueArgs FILES_VAR)
    set(multiValueArgs DEPENDENCIES)
    cmake_parse_arguments("${CMAKE_CURRENT_FUNCTION}" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(dependencies "${${CMAKE_CURRENT_FUNCTION}_DEPENDENCIES}")
    set(files_var "${${CMAKE_CURRENT_FUNCTION}_FILES_VAR}")
    # Clear the variable output list variable in the PARENT_SCOPE
    unset("${files_var}" PARENT_SCOPE)

    unset(files)
    foreach(dependency IN LISTS dependencies)
        unset(source_file)
        o3de_get_file_from_dependency(
            SOURCE_FILE_VAR source_file
            DEPENDENCY "${dependency}"
        )
        list(APPEND files "${source_file}")
    endforeach()

    if(files)
        set("${files_var}" "${files}" PARENT_SCOPE)
    endif()
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

        unset(LY_COPY_COMMANDS)
        unset(runtime_depends)

        unset(target_copy_dependencies)
        unset(target_target_dependencies)
        unset(target_link_dependencies)
        unset(target_imported_dependencies)
        o3de_get_dependencies_for_target(
            TARGET "${target}"
            COPY_DEPENDENCIES_VAR target_copy_dependencies
            TARGET_DEPENDENCIES_VAR target_target_dependencies
            LINK_DEPENDENCIES_VAR target_link_dependencies
            IMPORTED_DEPENDENCIES_VAR target_imported_dependencies
        )

        # Convert dependencies to files or generator expressions that can transform to files
        o3de_transform_dependencies_to_files(FILES_VAR target_copy_files
            DEPENDENCIES "${target_copy_dependencies}")
        o3de_transform_dependencies_to_files(FILES_VAR target_target_files
            DEPENDENCIES "${target_target_dependencies}")
        o3de_transform_dependencies_to_files(FILES_VAR target_link_files
            DEPENDENCIES "${target_link_dependencies}")
        o3de_transform_dependencies_to_files(FILES_VAR target_imported_files
            DEPENDENCIES "${target_imported_dependencies}")


        message(DEBUG "TARGET \"${target}\" has the following file dependencies:\n"
            "copy files -> \"${target_copy_files}\""
            "target files -> \"${target_target_files}\""
            "link files -> \"${target_copy_files}\""
            "imported files -> \"${target_copy_files}\"")

        foreach(dependency_for_target IN LISTS target_copy_dependencies
            target_link_dependencies
            target_target_dependencies
            target_imported_dependencies)
            unset(runtime_command)
            unset(runtime_depend)

            o3de_get_command_for_dependency(COMMAND_VAR runtime_command
                FILE_DEPENDENCY_VAR runtime_depend
                DEPENDENCY "${dependency_for_target}")
            string(APPEND LY_COPY_COMMANDS ${runtime_command})
            list(APPEND runtime_depends ${runtime_depend})
        endforeach()

        # Generate the output file, note the STAMP_OUTPUT_FILE need to match with the one defined in LYWrappers.cmake
        set(STAMP_OUTPUT_FILE ${CMAKE_BINARY_DIR}/runtime_dependencies/$<CONFIG>/${target}.stamp)

        unset(target_file_dir)
        unset(target_bundle_dir)
        unset(target_bundle_content_dir)
        # UTILITY type targets does not have a target file, so the CMAKE_RUNTIME_OUTPUT_DIRECTORY is used
        if (NOT target_type STREQUAL UTILITY)
            set(target_file_dir "$<TARGET_FILE_DIR:${target}>")
            set(target_file "$<TARGET_FILE:${target}>")

            # If the target is a bundle, configure the TARGET_BUNDLE_DIR
            # into the generated cmake file with runtime dependencies
            get_property(is_bundle TARGET ${target} PROPERTY MACOSX_BUNDLE)
            if(is_bundle)
                set(target_bundle_dir "$<TARGET_BUNDLE_DIR:${target}>")
                set(target_bundle_content_dir "$<TARGET_BUNDLE_CONTENT_DIR:${target}>")
            endif()
        else()
            set(target_file_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
            unset(target_file)
        endif()

        if(DEFINED ENV{USERPROFILE} AND EXISTS $ENV{USERPROFILE})
            set(PYTHON_ROOT_PATH "$ENV{USERPROFILE}/.o3de/Python") # Windows
        else()
            set(PYTHON_ROOT_PATH "$ENV{HOME}/.o3de/Python") # Unix
        endif()
        set(PYTHON_PACKAGES_ROOT_PATH "${PYTHON_ROOT_PATH}/packages")
        cmake_path(NORMAL_PATH PYTHON_PACKAGES_ROOT_PATH )

        set(LY_CURRENT_PYTHON_PACKAGE_PATH "${PYTHON_PACKAGES_ROOT_PATH}/${LY_PYTHON_PACKAGE_NAME}")
        cmake_path(NORMAL_PATH LY_CURRENT_PYTHON_PACKAGE_PATH )

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
