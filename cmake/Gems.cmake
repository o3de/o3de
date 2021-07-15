#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This file contains utility wrappers for dealing with the Gems system. 

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
    # the namespaced and non namespaced one, point at it.
    list(LENGTH ly_create_alias_TARGETS number_of_targets)
    if (number_of_targets EQUAL 1)
        if(TARGET ${ly_create_alias_TARGETS})
            ly_de_alias_target(${ly_create_alias_TARGETS} de_aliased_target_name)
            add_library(${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME} ALIAS ${de_aliased_target_name})
            if (NOT TARGET ${ly_create_alias_NAME})
                add_library(${ly_create_alias_NAME} ALIAS ${de_aliased_target_name})
            endif()
            # Store off the arguments needed used ly_create_alias into a DIRECTORY property
            # This will be used to re-create the calls in the generated CMakeLists.txt in the INSTALL step
            string(REPLACE ";" " " create_alias_args "${ly_create_alias_NAME},${ly_create_alias_NAMESPACE},${ly_create_alias_TARGETS}")
            set_property(DIRECTORY APPEND PROPERTY LY_CREATE_ALIAS_ARGUMENTS "${ly_create_alias_NAME},${ly_create_alias_NAMESPACE},${ly_create_alias_TARGETS}")
            return()
        endif()
    endif()

    # more complex version - one alias to multiple targets or the alias is being made to a TARGET that doesn't exist yet.
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
    endif()

    # now add the final alias:
    add_library(${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME} ALIAS ${ly_create_alias_NAME})

    # Store off the arguments used by ly_create_alias into a DIRECTORY property
    # This will be used to re-create the calls in the generated CMakeLists.txt in the INSTALL step

    # Replace the CMake list separator with a space to replicate the space separated TARGETS arguments
    string(REPLACE ";" " " create_alias_args "${ly_create_alias_NAME},${ly_create_alias_NAMESPACE},${ly_create_alias_TARGETS}")
    set_property(DIRECTORY APPEND PROPERTY LY_CREATE_ALIAS_ARGUMENTS "${create_alias_args}")
endfunction()

# ly_enable_gems
# this function makes sure that the given gems, or gems listed in the variable ENABLED_GEMS
# in the GEM_FILE name, are set as runtime dependencies (and thus loaded) for the given targets
# in the context of the given project.
# note that it can't do this immediately, so it saves the data for later processing.
# Note: If you don't supply a project name, it will apply it across the board to all projects.
# this is useful in the case of "ly_add_gems being called for so called 'mandatory gems' inside the engine.
# if you specify a gem name with a namespace, it will be used, otherwise it will assume Gem::
function(ly_enable_gems)
    set(options)
    set(oneValueArgs PROJECT_NAME GEM_FILE)
    set(multiValueArgs GEMS TARGETS VARIANTS)

    cmake_parse_arguments(ly_enable_gems "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ly_enable_gems_TARGETS)
        message(FATAL_ERROR "You must provide the targets to add gems to using the TARGETS keyword")
    endif()
       

    if (NOT ly_enable_gems_PROJECT_NAME)
        message(VERBOSE "Note: ly_enable_gems called with no PROJECT_NAME name, applying to all projects: \n"
            "   - VARIANTS ${ly_enable_gems_VARIANTS} \n"
            "   - GEMS     ${ly_enable_gems_GEMS} \n" 
            "   - TARGETS  ${ly_enable_gems_TARGETS} \n"
            "   - GEM_FILE ${ly_enable_gems_GEM_FILE}")
        set(ly_enable_gems_PROJECT_NAME "__NOPROJECT__") # so that the token is not blank
    endif()

    if (NOT ly_enable_gems_VARIANTS)
        message(FATAL_ERROR "You must provide at least 1 variant of the gem modules (Editor, Server, Client, Builder) to "
                            "add to your targets, using the VARIANTS keyword")
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
    foreach(target_name ${ly_enable_gems_TARGETS})
        foreach(variant_name ${ly_enable_gems_VARIANTS})
            set_property(GLOBAL APPEND PROPERTY LY_DELAYED_ENABLE_GEMS "${ly_enable_gems_PROJECT_NAME},${target_name},${variant_name}")
            define_property(GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS_"${ly_enable_gems_PROJECT_NAME},${target_name},${variant_name}"
                BRIEF_DOCS "List of gem names to evaluate variants against" FULL_DOCS "Names of gems that will be paired with the variant name
                    to determine if it is valid target that should be added as an application dynamic load dependency")
            set_property(GLOBAL APPEND PROPERTY LY_DELAYED_ENABLE_GEMS_"${ly_enable_gems_PROJECT_NAME},${target_name},${variant_name}" ${ly_enable_gems_GEMS})
        endforeach()
    endforeach()

    # Store off the arguments used by ly_enable_gems into a DIRECTORY property
    # This will be used to re-create the ly_enable_gems call in the generated CMakeLists.txt at the INSTALL step

    # Replace the CMake list separator with a space to replicate the space separated TARGETS arguments
    if(NOT ly_enable_gems_PROJECT_NAME STREQUAL "__NOPROJECT__")
        set(replicated_project_name ${ly_enable_gems_PROJECT_NAME})
    endif()
    # The GEM_FILE file is used to populate the GEMS argument via the ENABLED_GEMS variable in the file.
    # Furthermore the GEM_FILE itself is not copied over to the install layout, so make its argument entry blank and use the list of GEMS
    # stored in ly_enable_gems_GEMS
    string(REPLACE ";" " " enable_gems_args "${replicated_project_name},${ly_enable_gems_GEMS},,${ly_enable_gems_VARIANTS},${ly_enable_gems_TARGETS}")
    set_property(DIRECTORY APPEND PROPERTY LY_ENABLE_GEMS_ARGUMENTS "${enable_gems_args}")
endfunction()

# call this before runtime dependencies are used to add any relevant targets
# saved by the above function
function(ly_enable_gems_delayed)
    get_property(ly_delayed_enable_gems GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS)
    foreach(project_target_variant ${ly_delayed_enable_gems})
        # we expect a colon separated list of
        # PROJECT_NAME,target_name,variant_name
        string(REPLACE "," ";" project_target_variant_list "${project_target_variant}")
        list(LENGTH project_target_variant_list project_target_variant_length)
        if(project_target_variant_length EQUAL 0)
            continue()
        endif()
        
        if(NOT project_target_variant_length EQUAL 3)
            message(FATAL_ERROR "Invalid specification of gems, expected 'project','target','variant' and got ${project_target_variant}")
        endif()

        list(POP_BACK project_target_variant_list variant)
        list(POP_BACK project_target_variant_list target)
        list(POP_BACK project_target_variant_list project)

        get_property(gem_dependencies GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS_"${project_target_variant}")
        if (NOT gem_dependencies)
            get_property(gem_dependencies_defined GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS_"${project_target_variant}" DEFINED)
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

        if(${project} STREQUAL "__NOPROJECT__")
            # special case, apply to all
            unset(PREFIX_CLAUSE)
        else()
            set(PREFIX_CLAUSE "PREFIX;${project}")
        endif()

        if (NOT TARGET ${target})
            message(FATAL_ERROR "ly_enable_gems specified TARGET '${target}' but no such target was found.")
        endif()

        # apply the list of gem targets.  Adding a gem really just means adding the appropriate dependency.
        foreach(gem_name ${gem_dependencies})
            # the gem name may already have a namespace.  If it does, we use that one
            ly_strip_target_namespace(TARGET ${gem_name} OUTPUT_VARIABLE unaliased_gem_name)
            if (${unaliased_gem_name} STREQUAL ${gem_name})
                # if stripping a namespace had no effect, it had no namespace
                # and we supply the default Gem:: namespace.
                set(gem_name_with_namespace Gem::${gem_name})
            else()
                # if stripping the namespace had an effect then we use the original
                # with the namespace, instead of assuming Gem::
                set(gem_name_with_namespace ${gem_name})
            endif()
            
            # if the target exists, add it.
            if (TARGET ${gem_name_with_namespace}.${variant})
                ly_add_target_dependencies(
                    ${PREFIX_CLAUSE}
                    TARGETS ${target}
                    DEPENDENT_TARGETS ${gem_name_with_namespace}.${variant}
                )
            endif()
        endforeach()
    endforeach()
endfunction()
