#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This file contains utility wrappers for dealing with the Gems system.

set(GEM_VARIANT_DEFAULT_HeadlessServers Servers)

define_property(TARGET PROPERTY LY_PROJECT_NAME
    BRIEF_DOCS "Name of the project, this target can use enabled gems from"
    FULL_DOCS "If set, the when iterating over the enabled gems in ly_enabled_gems_delayed
    only a project with that name can have it's enabled gem list added as a dependency to this target.
    If the __NOPROJECT__ placeholder is associated with a list enabled gems, then it applies to this target regardless of this property value")


# o3de_gem_setup: Runs a CMAKE macro that reads the nearest ancestor gem.json
# of where the calling CMakeLists.txt is located and queries information
# about the Gem such as the Gem name and version
# NOTE: A CMake `macro` is used and not a `function` as it a macro
# executes in the same scope of the caller
# https://cmake.org/cmake/help/latest/command/macro.html#macro-vs-function
#
# \param:default_gem_name - Name to use for the ${gem_name} variable
#        if a gem.json file cannot be found by searching ancestor directories
#        or if the "gem_json" file does not contain a "gem_name" field
#
# \return:gem_path - Sets the ${gem_path} variable to the directory containing
#         the gem.json that is the nearest ancestor directory of the calling CMakeLists.txt.
#         The "nearest ancestor" also includes the current CMakeLists.txt for clarification
# \return:gem_name - Sets the ${gem_name} variable to the "gem_name" field
# \return:gem_version - Sets the ${gem_version} variable to the "gem_version" field
# \return:gem_restricted_path - Sets the ${gem_restricted_path} variable
#         to store the root directory of the Gem for the current platform if it is "restricted"
#         (i.e Console) platform.
# \return:gem_parent_relative_path - Sets the ${gem_parent_relative_path} variable
#         to the relative path  o the Gem root directory within the restricted
#         platform root directory
#         Ex.
#         /home/user/
#           o3de/
#             Gems/
#               RayTracing/
#                 gem.json
#             restricted_platform_name_which_cannot_be_mentioned/
#               Gems/
#                 RayTracing/
#                   restricted.json
# \return:pal_dir - Sets the ${pal_dir} variable to the "platform
#         abstracted layer" (PAL) version of the current directory for a specific platform
#         For example if the directory containing the calling CMakeLists.txt
#         is "Gems/Location/",
#         then the PAL directory for windows is "Gems/Location/Platform/Windows/".
#         For a restricted platfrom, the directory could be located at the path
#         "<restricted-platform-root>/Gems/Location/Platform/<platform>/"
macro(o3de_gem_setup default_gem_name)
    # Query the gem name from the gem.json file if possible
    # otherwise fallback to using the default gem name argument
    o3de_find_ancestor_gem_root(gem_path gem_name "${CMAKE_CURRENT_SOURCE_DIR}")
    if (NOT gem_name)
        set(gem_name "${default_gem_name}")
    endif()

    # Fallback to using the current source CMakeLists.txt directory as the gem root path
    if (NOT gem_path)
        set(gem_path ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    set(gem_json ${gem_path}/gem.json)

    # Read the version field from the gem.json
    set(gem_version, "0.0.0")
    o3de_read_json_key(gem_version ${gem_json} "version")

    o3de_restricted_path(${gem_json} gem_restricted_path gem_parent_relative_path)

    o3de_pal_dir(pal_dir ${CMAKE_CURRENT_SOURCE_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
endmacro()

# get_all_gem_dependencies
#
# Determine all of the gem dependencies (recursively) for a given gem.
#
# \arg:gem_name(STRING) - Gem name whose "dependencies" will be queried from its gem.json
# \arg:output_resolved_gem_names(LIST) - The updated list of resolved gem name dependencies
function(get_all_gem_dependencies gem_name output_resolved_gem_names)

    get_property(gem_dependencies GLOBAL PROPERTY GEM_DEPENDENCIES_"${gem_name}")
    if (gem_dependencies)
        # The gem dependency for ${gem_name} has been calculated, return the cached property
        set(${output_resolved_gem_names} ${gem_dependencies} PARENT_SCOPE)
        return()
    else()
        # The gem dependency for ${gem_name} has not been calculated. 
        # First read in the dependencies from the gem.json if possible

        # Strip out any possible version specifier in order to lookup the gem path based on the cached
        # global property "@GEMROOT:${gem_name}"
        unset(gem_name_only)
        o3de_get_name_and_version_specifier(${gem_name} gem_name_only ignore_spec_op ignore_spec_version)
        get_property(gem_path GLOBAL PROPERTY "@GEMROOT:${gem_name_only}@")
        if (NOT gem_path)
            unset(gem_optional)
            get_property(gem_optional GLOBAL PROPERTY ${gem_name}_OPTIONAL)
            if (gem_optional)
                return()
            endif()
            message(FATAL_ERROR "Unable to locate gem path for Gem \"${gem_name_only}\"."
                    " Is the gem registered in either the ~/.o3de/o3de_manifest.json, ${LY_ROOT_FOLDER}/engine.json,"
                    " any project.json or any gem.json which itself is registered?")
        endif()

        o3de_read_json_array(gem_dependencies "${gem_path}/gem.json" "dependencies")

        # Keep track of the visited gems to prevent any dependency cycles that can
        # cause a possible infinite recursion when evaluating the gem dependencies' dependency
        list(APPEND get_all_gem_dependencies_visited ${gem_name})

        # Iterate and recursively collect dependent gem names
        unset(all_dependent_gem_names)
        foreach(dependent_gem_name IN LISTS gem_dependencies)
            unset(dependent_gem_names)

            # Recursively check a depend gem only if it has not been visited yet
            unset(visited_index)
            list(FIND get_all_gem_dependencies_visited ${dependent_gem_name} visited_index)
            if (visited_index LESS 0)
                get_all_gem_dependencies(${dependent_gem_name} dependent_gem_names)
                list(APPEND all_dependent_gem_names ${dependent_gem_names})
            endif()
        endforeach()
        list(APPEND gem_dependencies ${all_dependent_gem_names})
        list(REMOVE_DUPLICATES gem_dependencies)

        # Update the cached value and set the results
        set_property(GLOBAL PROPERTY GEM_DEPENDENCIES_"${gem_name}" ${gem_dependencies})
        set(${output_resolved_gem_names} ${gem_dependencies} PARENT_SCOPE)
    endif()

endfunction()

# o3de_find_ancestor_gem_root:Searches for the nearest gem root from input source_dir
#
# \arg:source_dir(FILEPATH) - Filepath to walk upwards from to locate a gem.json
# \return:output_gem_module_root - The directory containing the nearest gem.json
# \return:output_gem_name - The name of the gem read from the gem.json
function(o3de_find_ancestor_gem_root output_gem_module_root output_gem_name source_dir)
    unset(${output_gem_module_root} PARENT_SCOPE)
    
    if(source_dir)
        set(candidate_gem_path ${source_dir})
        # Locate the root of the gem by finding the gem.json location
        cmake_path(APPEND candidate_gem_path "gem.json" OUTPUT_VARIABLE candidate_gem_json_path)
        while(NOT EXISTS "${candidate_gem_json_path}")
            cmake_path(GET candidate_gem_path PARENT_PATH parent_path)

            # If the parent directory is the same as the candidate path then the root path has been found
            cmake_path(COMPARE "${candidate_gem_path}" EQUAL "${parent_path}" reached_root_dir)
            if (reached_root_dir)
                # The source directory is not under a gem path in this case
                return()
            endif()
            set(candidate_gem_path ${parent_path})
            cmake_path(APPEND candidate_gem_path "gem.json" OUTPUT_VARIABLE candidate_gem_json_path)
        endwhile()
    endif()

    if (EXISTS ${candidate_gem_json_path})
        # Update source_dir if the gem root path exists
        set(source_dir ${candidate_gem_path})
        o3de_read_json_key(gem_name ${candidate_gem_json_path} "gem_name")

        unset(gem_provided_service)
        o3de_read_optional_json_key(gem_provided_service ${candidate_gem_json_path} "provided_unique_service")
    endif()

    # Set the gem module root output directory to the location with the gem.json file within it or
    # the supplied gem_target SOURCE_DIR location if no gem.json file was found
    set(${output_gem_module_root} ${source_dir} PARENT_SCOPE)

    # Set the gem name output value to the name of the gem as in the gem.json file
    if(gem_name)
        set(${output_gem_name} ${gem_name} PARENT_SCOPE)
    endif()

    if (gem_provided_service)
        # Track any (optional) provided unique service that this gem provides
        set_property(GLOBAL PROPERTY LY_GEM_PROVIDED_SERVICE_"${gem_name}" "${gem_provided_unique_service}")
    endif()

endfunction()

# o3de_add_variant_dependencies_for_gem_dependencies
#
# For the specified gem, creates cmake TARGET dependencies using
# the gem's "dependencies" field as a prefix for each gem variant supplied to this function
# \arg:GEM_NAME(STRING) - Gem name whose "dependencies" will be queried from its gem.json
# \arg:VARIANTS(LIST) - List of Gem variants which will be suffixed to the input gem name and
#      its gem dependencies
function(o3de_add_variant_dependencies_for_gem_dependencies)
    set(options)
    set(oneValueArgs GEM_NAME)
    set(multiValueArgs VARIANTS)

    cmake_parse_arguments(o3de_add_variant_dependencies_for_gem_dependencies "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(gem_name ${o3de_add_variant_dependencies_for_gem_dependencies_GEM_NAME})
    set(gem_variants ${o3de_add_variant_dependencies_for_gem_dependencies_VARIANTS})
    if(NOT gem_name OR NOT gem_variants)
        return() # Nothing to do
    endif()

    # Get the gem path using the gem name as a key
    get_property(gem_path GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
    if(NOT gem_path)
        message(FATAL_ERROR "Unable to locate gem path for Gem \"${gem_name}\"."
        " Is the gem registered in either the ~/.o3de/o3de_manifest.json, ${LY_ROOT_FOLDER}/engine.json,"
        " any project.json or any gem.json which itself is registered?")
    endif()

    # Open gem.json and read "dependencies" array
    unset(gem_dependencies)
    get_all_gem_dependencies(${gem_name} gem_dependencies)

    foreach(variant IN LISTS gem_variants)
        set(gem_variant_target "${gem_name}.${variant}")
        # Continue to the next variant if the gem didn't specify
        # a CMake target for the current variant
        if(NOT TARGET ${gem_variant_target})
            continue()
        endif()

        # Append to the list of target dependencies for the current list
        # CMake targets that actually exists
        unset(target_dependencies_for_variant)
        foreach(gem_dependency IN LISTS gem_dependencies)
            set(gem_dependency_variant_target "${gem_dependency}.${variant}")
            if(TARGET ${gem_dependency_variant_target})
                list(APPEND target_dependencies_for_variant ${gem_dependency_variant_target})
            endif()
        endforeach()
        # Validate that there is at least one dependency being added
        if(target_dependencies_for_variant)
            ly_add_dependencies(${gem_variant_target} ${target_dependencies_for_variant})
            message(VERBOSE "Adding target dependencies for Gem \"${gem_name}\" by appending variant \"${variant}\""
                " to gem names found in this gem's \"dependencies\" field\n"
                "${gem_variant_target} -> ${gem_dependencies_for_variant}")
        endif()
    endforeach()

    # Store of the arguments used to invoke this function in order to replicate the call
    # in the SDK install layout
    unset(add_variant_dependencies_for_gem_dependencies_args)
    list(APPEND add_variant_dependencies_for_gem_dependencies_args
        GEM_NAME ${gem_name}
        VARIANTS ${gem_variants})
    # Replace the list separator with space to have it be stored as a single property element
    list(JOIN add_variant_dependencies_for_gem_dependencies_args " " add_variant_dependencies_for_gem_dependencies_args)
    set_property(DIRECTORY APPEND PROPERTY O3DE_ADD_VARIANT_DEPENDENCIES_FOR_GEM_DEPENDENCIES_ARGUMENTS
        "${add_variant_dependencies_for_gem_dependencies_args}")
endfunction()

# ly_create_alias
# given an alias to create, and a list of one or more targets,
# this creates an alias that depends on all of the given targets.
function(ly_create_alias)
    set(options)
    set(oneValueArgs NAME NAMESPACE)
    set(multiValueArgs TARGETS)

    cmake_parse_arguments(ly_create_alias "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ly_create_alias_NAME)
        message(FATAL_ERROR "Provide the name of the alias to create using the NAME keyword")
    endif()

    if (NOT ly_create_alias_NAMESPACE)
        message(FATAL_ERROR "Provide the namespace of the alias to create using the NAMESPACE keyword")
    endif()


    if(TARGET ${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME})
        message(FATAL_ERROR "Target already exists, cannot create an alias for it: ${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME}\n"
                            "Make sure the target wasn't copy and pasted here or elsewhere.")
    endif()


    # Using an ALIAS target inhibits finding the ALIAS target SOURCE_DIR, which is needed to determine the gem root of the alias target

    # complex version - one alias to multiple targets or the alias is being made to a TARGET that doesn't exist yet.
    # To actually achieve this we have to create an interface library with those dependencies,
    # then we have to create an alias to that target.
    # By convention we create one without a namespace then alias the namespaced one.
    if(TARGET ${ly_create_alias_NAME})
        message(FATAL_ERROR "Internal alias target already exists, cannot create an alias for it: ${ly_create_alias_NAME}\n"
                            "This could be a copy-paste error, where some part of the ly_create_alias call was changed but the other")
    endif()

    add_library(${ly_create_alias_NAME} INTERFACE IMPORTED GLOBAL)
    set_target_properties(${ly_create_alias_NAME} PROPERTIES GEM_MODULE TRUE)

    foreach(target_name ${ly_create_alias_TARGETS})
        if(TARGET ${target_name})
            ly_de_alias_target(${target_name} de_aliased_target_name)
            if(NOT de_aliased_target_name)
                message(FATAL_ERROR "Target not found in ly_create_alias call: ${target_name} - check your spelling of the target name")
            endif()
        else()
            set(de_aliased_target_name ${target_name})
        endif()
        list(APPEND final_targets ${de_aliased_target_name})
    endforeach()

    # add_dependencies must be called with at least one dependent target
    if(final_targets)
        ly_parse_third_party_dependencies("${final_targets}")
        ly_add_dependencies(${ly_create_alias_NAME} ${final_targets})
        # copy over all the dependent target interface properties to the alias
        o3de_copy_targets_usage_requirements(TARGET ${ly_create_alias_NAME} SOURCE_TARGETS ${final_targets})
        # Register the targets this alias aliases
        set_property(GLOBAL APPEND PROPERTY O3DE_ALIASED_TARGETS_${ly_create_alias_NAME} ${final_targets})
    endif()

    # now add the final alias:
    add_library(${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME} ALIAS ${ly_create_alias_NAME})


    # Store off the arguments used by ly_create_alias into a DIRECTORY property
    # This will be used to re-create the calls in the generated CMakeLists.txt in the INSTALL step
    # Replace the CMake list separator with a space to replicate the space separated arguments
    # A single create_alias_args variable encodes two values. The alias NAME used to check if the target exists
    # and the ly_create_alias arguments to replicate this function call
    unset(create_alias_args)
    list(APPEND create_alias_args "${ly_create_alias_NAME},"
        NAME ${ly_create_alias_NAME}
        NAMESPACE ${ly_create_alias_NAMESPACE}
        TARGETS ${ly_create_alias_TARGETS})
    list(JOIN create_alias_args " " create_alias_args)
    set_property(DIRECTORY APPEND PROPERTY LY_CREATE_ALIAS_ARGUMENTS "${create_alias_args}")

    # Store the directory path in the GLOBAL property so that it can be accessed
    # in the layout install logic. Skip if the directory has already been added
    get_property(ly_all_target_directories GLOBAL PROPERTY LY_ALL_TARGET_DIRECTORIES)
    if(NOT CMAKE_CURRENT_SOURCE_DIR IN_LIST ly_all_target_directories)
        set_property(GLOBAL APPEND PROPERTY LY_ALL_TARGET_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
endfunction()

# ly_set_gem_variant_to_load
# Associates a key, value entry of CMake target -> Gem variant
# \arg:TARGETS - list of Targets to associate with the Gem variant
# \arg:VARIANTS - Gem variant
function(ly_set_gem_variant_to_load)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs TARGETS VARIANTS)

    cmake_parse_arguments(ly_set_gem_variant_to_load "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ly_set_gem_variant_to_load_TARGETS)
        message(FATAL_ERROR "You must provide at least 1 target to ${CMAKE_CURRENT_FUNCTION} using the TARGETS keyword")
    endif()

    # Store a list of targets
    foreach(target_name ${ly_set_gem_variant_to_load_TARGETS})
        # Append the target to the list of targets with variants if it has not been added
        get_property(ly_targets_with_variants GLOBAL PROPERTY LY_TARGETS_WITH_GEM_VARIANTS)
        if(NOT target_name IN_LIST ly_targets_with_variants)
            set_property(GLOBAL APPEND PROPERTY LY_TARGETS_WITH_GEM_VARIANTS "${target_name}")
        endif()
        foreach(variant_name ${ly_set_gem_variant_to_load_VARIANTS})
            get_property(target_gem_variants GLOBAL PROPERTY LY_GEM_VARIANTS_"${target_name}")
            if(NOT variant_name IN_LIST target_gem_variants)
                set_property(GLOBAL APPEND PROPERTY LY_GEM_VARIANTS_"${target_name}" "${variant_name}")
            endif()
        endforeach()
    endforeach()

    # Store of the arguments used to invoke this function in order to replicate the call in the generated CMakeLists.txt
    # in the install layout
    unset(set_gem_variant_args)
    list(APPEND set_gem_variant_args
        TARGETS ${ly_set_gem_variant_to_load_TARGETS}
        VARIANTS ${ly_set_gem_variant_to_load_VARIANTS})
    # Replace the list separator with space to have it be stored as a single property element
    list(JOIN set_gem_variant_args " " set_gem_variant_args)
    set_property(DIRECTORY APPEND PROPERTY LY_SET_GEM_VARIANT_TO_LOAD_ARGUMENTS "${set_gem_variant_args}")
endfunction()

# ly_enable_gems
# this function makes sure that the given gems, or gems listed in the variable ENABLED_GEMS
# in the GEM_FILE name, are set as runtime dependencies (and thus loaded) for the given targets
# in the context of the given project.
# note that it can't do this immediately, so it saves the data for later processing.
# Note: If you don't supply a project name, it will apply it across the board to all projects.
# this is useful in the case of "ly_enable_gems" being called for so called 'mandatory gems' inside the engine.
# if you specify a gem name with a namespace, it will be used, otherwise it will assume Gem::
function(ly_enable_gems)
    set(options)
    set(oneValueArgs PROJECT_NAME GEM_FILE)
    set(multiValueArgs GEMS TARGETS VARIANTS)

    cmake_parse_arguments(ly_enable_gems "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ly_enable_gems_PROJECT_NAME)
        message(VERBOSE "Note: ly_enable_gems called with no PROJECT_NAME name, applying to all projects: \n"
            "   - GEMS     ${ly_enable_gems_GEMS} \n"
            "   - GEM_FILE ${ly_enable_gems_GEM_FILE}")
        set(ly_enable_gems_PROJECT_NAME "__NOPROJECT__") # so that the token is not blank
    endif()

    # Backwards-Compatibility - Delegate any TARGETS and VARIANTS arguments to the ly_set_gem_variant_to_load
    # command. That command is used to associate TARGETS with the list of Gem Variants they desire to use
    if (ly_enable_gems_TARGETS AND ly_enable_gems_VARIANTS)
        message(DEPRECATION "The TARGETS and VARIANTS arguments to \"${CMAKE_CURRENT_FUNCTION}\" is deprecated.\n"
            "Please use the \"ly_set_gem_variant_to_load\" function directly to associate a Target with a Gem Variant.\n"
            "This function will forward the TARGETS and VARIANTS arguments to \"ly_set_gem_variant_to_load\" for now,"
            " but this functionality will be removed.")
        ly_set_gem_variant_to_load(TARGETS ${ly_enable_gems_TARGETS} VARIANTS ${ly_enable_gems_VARIANTS})
    endif()

    if ((NOT ly_enable_gems_GEMS AND NOT ly_enable_gems_GEM_FILE) OR (ly_enable_gems_GEMS AND ly_enable_gems_GEM_FILE))
        message(FATAL_ERROR "Provide exactly one of either GEM_FILE (filename) or GEMS (list of gems) keywords.")
    endif()

    if (ly_enable_gems_GEM_FILE)
        set(store_temp ${ENABLED_GEMS})
        include(${ly_enable_gems_GEM_FILE} RESULT_VARIABLE was_able_to_load_the_file)
        if(NOT was_able_to_load_the_file)
            message(FATAL_ERROR "could not load the GEM_FILE ${ly_enable_gems_GEM_FILE}")
        endif()
        if(NOT DEFINED ENABLED_GEMS)
            message(WARNING "GEM_FILE ${ly_enable_gems_GEM_FILE} did not set the value of ENABLED_GEMS.\n"
                                "Gem Files should contain set(ENABLED_GEMS ... <list of gem names>)")
        endif()
        set(ly_enable_gems_GEMS ${ENABLED_GEMS})
        set(ENABLED_GEMS ${store_temp}) # restore value of ENABLED_GEMS just in case...
    endif()

    # Remove any version specifiers before looking for variants
    # e.g. "Atom==1.2.3" becomes "Atom"
    unset(GEM_NAMES)
    unset(additional_dependent_gems)
    foreach(gem_name_with_version_specifier IN LISTS ly_enable_gems_GEMS)
        o3de_get_name_and_version_specifier(${gem_name_with_version_specifier} gem_name spec_op spec_version)
        list(APPEND GEM_NAMES "${gem_name}")
        # In addition to the gems enabled, collect each of the enabled gem's dependencies as well
        unset(gem_dependencies_for_gem)
        get_all_gem_dependencies(${gem_name} gem_dependencies_for_gem)
        list(APPEND additional_dependent_gems ${gem_dependencies_for_gem})
    endforeach()

    # Update the gems that were enabled to include all of the gem's dependent gems as well
    list(APPEND GEM_NAMES "${additional_dependent_gems}")
    list(REMOVE_DUPLICATES GEM_NAMES)
    set(ly_enable_gems_GEMS ${GEM_NAMES})

    # all the actual work has to be done later.
    set_property(GLOBAL APPEND PROPERTY LY_DELAYED_ENABLE_GEMS "${ly_enable_gems_PROJECT_NAME}")
    define_property(GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS_"${ly_enable_gems_PROJECT_NAME}"
        BRIEF_DOCS "List of gem names to evaluate variants against" FULL_DOCS "Names of gems that will be paired with the variant name
            to determine if it is valid target that should be added as an application dynamic load dependency")
    set_property(GLOBAL APPEND PROPERTY LY_DELAYED_ENABLE_GEMS_"${ly_enable_gems_PROJECT_NAME}" ${ly_enable_gems_GEMS})

    # Store off the arguments used by ly_enable_gems into a DIRECTORY property
    # This will be used to re-create the ly_enable_gems call in the generated CMakeLists.txt at the INSTALL step

    # Replace the CMake list separator with a space to replicate the space separated TARGETS arguments
    if(NOT ly_enable_gems_PROJECT_NAME STREQUAL "__NOPROJECT__")
        set(replicated_project_name PROJECT_NAME ${ly_enable_gems_PROJECT_NAME})
    endif()
    # The GEM_FILE file is used to populate the GEMS argument via the ENABLED_GEMS variable in the file.
    # Furthermore the GEM_FILE itself is not copied over to the install layout, so make its argument entry blank and use the list of GEMS
    # stored in ly_enable_gems_GEMS
    unset(enable_gems_args)
    list(APPEND enable_gems_args
        ${replicated_project_name}
        GEMS ${ly_enable_gems_GEMS})
    list(JOIN enable_gems_args " " enable_gems_args)
    set_property(DIRECTORY APPEND PROPERTY LY_ENABLE_GEMS_ARGUMENTS "${enable_gems_args}")
endfunction()


function(ly_add_gem_dependencies_to_project_variants)
    set(options)
    set(oneValueArgs PROJECT_NAME TARGET VARIANT)
    set(multiValueArgs GEM_DEPENDENCIES)

    cmake_parse_arguments(ly_add_gem_dependencies "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT ly_add_gem_dependencies_PROJECT_NAME)
        message(FATAL_ERROR "Missing required PROJECT_NAME argument which is used to determine gem load prefix")
    endif()
    if (NOT ly_add_gem_dependencies_TARGET)
        message(FATAL_ERROR "Missing required TARGET argument ")
    endif()
    if (NOT ly_add_gem_dependencies_VARIANT)
        message(FATAL_ERROR "Missing required gem VARIANT argument needed to determine which gem variants to load for the target")
    endif()

    if(${ly_add_gem_dependencies_PROJECT_NAME} STREQUAL "__NOPROJECT__")
        # special case, apply to all
        unset(PREFIX_CLAUSE)
    else()
        set(PREFIX_CLAUSE "PREFIX;${ly_add_gem_dependencies_PROJECT_NAME}")
    endif()

    # apply the list of gem targets.  Adding a gem really just means adding the appropriate dependency.
    foreach(gem_name ${ly_add_gem_dependencies_GEM_DEPENDENCIES})

        # Construct the gem target name to determine if the target exists
        set(gem_target ${gem_name}.${ly_add_gem_dependencies_VARIANT})

        # Also construct a fallback target if the target doesn't exist for the given variant, then check if the variant has a fallback
        set(fallback_gem_target ${gem_name}.${GEM_VARIANT_DEFAULT_${ly_add_gem_dependencies_VARIANT}})

        if (TARGET ${gem_target})
            ly_de_alias_target(${gem_target} dealiased_gem_target)
        elseif (TARGET ${fallback_gem_target})
            ly_de_alias_target(${fallback_gem_target} dealiased_gem_target)
        else()
            set(dealiased_gem_target "")
            message(VERBOSE "Gem \"${gem_name}\" does not expose a variant of ${ly_add_gem_dependencies_VARIANT}")
        endif()

        if (NOT "${dealiased_gem_target}" STREQUAL "")
            ly_add_target_dependencies(
                ${PREFIX_CLAUSE}
                TARGETS ${ly_add_gem_dependencies_TARGET}
               DEPENDENT_TARGETS ${dealiased_gem_target}
                GEM_VARIANT ${ly_add_gem_dependencies_VARIANT})
        endif()
    endforeach()
endfunction()

# call this before runtime dependencies are used to add any relevant targets
# saved by the above function
function(ly_enable_gems_delayed)
    # Query the list of targets that are associated with a gem variant
    get_property(targets_with_variants GLOBAL PROPERTY LY_TARGETS_WITH_GEM_VARIANTS)
    # Query the projects that have made calls to ly_enable_gems
    get_property(enable_gem_projects GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS)

    foreach(target ${targets_with_variants})
        if (NOT TARGET ${target})
            message(FATAL_ERROR "ly_set_gem_variant_to_load specified TARGET '${target}' but no such target was found.")
        endif()

        get_property(target_gem_variants GLOBAL PROPERTY LY_GEM_VARIANTS_"${target}")
        message(VERBOSE "Adding gem dependencies for \"${target}\" associated with the Gem variants of \"${target_gem_variants}\"")

        # Lookup if the target is scoped to a project
        # In that case the target can only use gem targets that is
        # - not project specific: i.e "__NOPROJECT__"
        # - or specific to the <LY_PROJECT_NAME> project
        get_property(target_project_association TARGET ${target} PROPERTY LY_PROJECT_NAME)

        foreach(project ${enable_gem_projects})
            if (target_project_association AND
                (NOT (project STREQUAL "__NOPROJECT__") AND NOT (project STREQUAL target_project_association)))
                # Skip adding the gem dependencies to this target if it is associated with a project
                # and the current project doesn't match
                continue()
            endif()

            get_property(gem_dependencies GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS_"${project}")
            if (NOT gem_dependencies)
                get_property(gem_dependencies_defined GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS_"${project}" DEFINED)
                if (gem_dependencies_defined)
                    # special case, if the LY_DELAYED_ENABLE_GEMS_"${project_target_variant}" property is DEFINED
                    # but empty, add an entry to the LY_DELAYED_LOAD_DEPENDENCIES to have the
                    # cmake_dependencies.*.setreg file for the (project, target) tuple to be regenerated
                    # This is needed if the ENABLED_GEMS list for a project goes from >0 to 0. In this case
                    # the cmake_dependencies would have a stale list of gems to load unless it is regenerated
                    foreach(variant IN LISTS target_gem_variants)
                        get_property(delayed_load_target_set GLOBAL PROPERTY LY_DELAYED_LOAD_"${project},${target},${variant}" SET)
                        if(NOT delayed_load_target_set)
                            set_property(GLOBAL APPEND PROPERTY LY_DELAYED_LOAD_DEPENDENCIES "${project},${target},${variant}")
                            set_property(GLOBAL APPEND PROPERTY LY_DELAYED_LOAD_"${project},${target},${variant}" "")
                        endif()
                    endforeach()
                endif()
                # Continue to the next iteration loop regardless as there are no gem dependencies
                continue()
            endif()

            # Gather the Gem variants associated with this target and iterate over them to combine them with the enabled
            # gems for the each project
            foreach(variant IN LISTS target_gem_variants)
                ly_add_gem_dependencies_to_project_variants(
                    PROJECT_NAME ${project}
                    TARGET ${target}
                    VARIANT ${variant}
                    GEM_DEPENDENCIES ${gem_dependencies})
            endforeach()
        endforeach()

        # Make sure that there are no targets that have dependencies on multiple gems that provide the same unique service
        foreach(project ${enable_gem_projects})

            # Skip __NOPROJECT__ since the gems won't be loaded anyways
            if (project STREQUAL "__NOPROJECT__")
                continue()
            endif()

            # Get the enabled gems for ${project} and track any unique service
            get_property(enabled_gems_for_project GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS_"${project}")
            unset(identified_unique_services)
            foreach(dep_gem ${enabled_gems_for_project})
                get_property(gem_provided_unique_service GLOBAL PROPERTY LY_GEM_PROVIDED_SERVICE_"${dep_gem}")
                if (gem_provided_unique_service)
                    get_property(servicing_gem GLOBAL PROPERTY unique_service_"${project}"_"${gem_provided_unique_service}")
                    if ((servicing_gem) AND (NOT "${dep_gem}" STREQUAL "${servicing_gem}"))
                        message(FATAL_ERROR "Target '${project}' detected conflicting gems that provide the same service '${gem_provided_unique_service}': "
                                            "${dep_gem}, ${servicing_gem}. Disable one of these gems or any gem that has a dependency for one of these "
                                             "gems to continue.")
                    else()
                        set_property(GLOBAL PROPERTY unique_service_"${project}"_"${gem_provided_unique_service}" ${dep_gem})
                    endif()
                endif()
            endforeach()
        endforeach()

    endforeach()
endfunction()
