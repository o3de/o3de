#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This file contains utility wrappers for dealing with the Gems system.

define_property(TARGET PROPERTY LY_PROJECT_NAME
    BRIEF_DOCS "Name of the project, this target can use enabled gems from"
    FULL_DOCS "If set, the when iterating over the enabled gems in ly_enabled_gems_delayed
    only a project with that name can have it's enabled gem list added as a dependency to this target.
    If the __NOPROJECT__ placeholder is associated with a list enabled gems, then it applies to this target regardless of this property value")

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

    # easy version - if its just one target and it exist at the time of this call,
    # we can directly get the target, and make both aliases,
    # the namespace and non namespace one, point at it.
    set(create_interface_target TRUE)
    list(LENGTH ly_create_alias_TARGETS number_of_targets)
    if (number_of_targets EQUAL 1)
        if(TARGET ${ly_create_alias_TARGETS})
            ly_de_alias_target(${ly_create_alias_TARGETS} de_aliased_target_name)
            add_library(${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME} ALIAS ${de_aliased_target_name})
            if (NOT TARGET ${ly_create_alias_NAME})
                add_library(${ly_create_alias_NAME} ALIAS ${de_aliased_target_name})
            endif()
            set(create_interface_target FALSE)
        endif()
    endif()

    # more complex version - one alias to multiple targets or the alias is being made to a TARGET that doesn't exist yet.
    # To actually achieve this we have to create an interface library with those dependencies,
    # then we have to create an alias to that target.
    # By convention we create one without a namespace then alias the namespaced one.
    if(create_interface_target)
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
        endif()

        # now add the final alias:
        add_library(${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME} ALIAS ${ly_create_alias_NAME})
    endif()

    # Store off the arguments used by ly_create_alias into a DIRECTORY property
    # This will be used to re-create the calls in the generated CMakeLists.txt in the INSTALL step
    # Replace the CMake list separator with a space to replicate the space separated arguments
    # A single create_alias_args variable encodes two values. The alias NAME used to check if the target exists
    # and the ly_create_alias arguments to replace this function call
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
        set(gem_target ${gem_name}.${ly_add_gem_dependencies_VARIANT})

        # if the target exists, add it.
        if (TARGET ${gem_target})
            # Dealias actual target
            ly_de_alias_target(${gem_target} dealiased_gem_target)
            ly_add_target_dependencies(
                ${PREFIX_CLAUSE}
                TARGETS ${ly_add_gem_dependencies_TARGET}
                DEPENDENT_TARGETS ${dealiased_gem_target})
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
                    get_property(delayed_load_target_set GLOBAL PROPERTY LY_DELAYED_LOAD_"${project},${target}" SET)
                    if(NOT delayed_load_target_set)
                        set_property(GLOBAL APPEND PROPERTY LY_DELAYED_LOAD_DEPENDENCIES "${project},${target}")
                        set_property(GLOBAL APPEND PROPERTY LY_DELAYED_LOAD_"${project},${target}" "")
                    endif()
                endif()
                # Continue to the next iteration loop regardless as there are no gem dependencies
                continue()
            endif()

            # Gather the Gem variants associated with this target and iterate over them to combine them with the enabled
            # gems for the each project
            get_property(target_gem_variants GLOBAL PROPERTY LY_GEM_VARIANTS_"${target}")
            foreach(variant ${target_gem_variants})
                ly_add_gem_dependencies_to_project_variants(
                    PROJECT_NAME ${project}
                    TARGET ${target}
                    VARIANT ${variant}
                    GEM_DEPENDENCIES ${gem_dependencies})
            endforeach()
        endforeach()
    endforeach()
endfunction()
