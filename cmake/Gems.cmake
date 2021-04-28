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

    if (NOT ly_create_alias_TARGETS)
        message(FATAL_ERROR "Provide the name of the targets the alias be associated with, using the TARGETS keyword")
    endif()

    if(TARGET ${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME})
        message(FATAL_ERROR "Target already exists, cannot create an alias for it: ${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME}\n"
                            "Make sure the target wasn't copy and pasted here or elsewhere.")
    endif()

    # easy version - if its juts one target, we can directly get the target, and make both aliases, 
    # the namespaced and non namespaced one, point at it.
    list(LENGTH ly_create_alias_TARGETS number_of_targets)
    if (number_of_targets EQUAL 1)
        ly_de_alias_target(${ly_create_alias_TARGETS} de_aliased_target_name)
        add_library(${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME} ALIAS ${de_aliased_target_name})
        if (NOT TARGET ${ly_create_alias_NAME})
            add_library(${ly_create_alias_NAME} ALIAS ${de_aliased_target_name})
        endif()
        return()
    endif()

    # more complex version - one alias to multiple targets.  To actually achieve this
    # we have to create an interface library with those dependencies, then we have to create an alias to that target.
    # by convention we create one without a namespace then alias the namespaced one.

    if(TARGET ${ly_create_alias_NAME})
        message(FATAL_ERROR "Internal alias target already exists, cannot create an alias for it: ${ly_create_alias_NAME}\n"
                            "This could be a copy-paste error, where some part of the ly_create_alias call was changed but the other")
    endif()

    add_library(${ly_create_alias_NAME} INTERFACE IMPORTED)
    set_target_properties(${ly_create_alias_NAME} PROPERTIES GEM_MODULE TRUE)

    foreach(target_name ${ly_create_alias_TARGETS})
        ly_de_alias_target(${target_name} de_aliased_target_name)
        if(NOT de_aliased_target_name)
            message(FATAL_ERROR "Target not found in ly_create_alias call: ${target_name} - check your spelling of the target name")
        endif()
        list(APPEND final_targets ${de_aliased_target_name})
    endforeach()
    
    ly_parse_third_party_dependencies("${final_targets}")
    ly_add_dependencies(${ly_create_alias_NAME} ${final_targets})

    # now add the final alias:
    add_library(${ly_create_alias_NAMESPACE}::${ly_create_alias_NAME} ALIAS ${ly_create_alias_NAME})
endfunction()

# ly_enable_gems
# this function makes sure that the given gems, or gems listed in the variable ENABLED_GEMS
# in the GEM_FILE name, are set as runtime dependencies (and thus loaded) for the given targets
# in the context of the given project.
# note that it can't do this immediately, so it saves the data for later processing.
# Note: If you don't supply a project name, it will apply it across the board to all projects.
# this is useful in the case of "ly_add_gems being called for so called 'mandatory gems' inside the engine.
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
        if(NOT ENABLED_GEMS)
            message(FATAL_ERROR "GEM_FILE ${ly_enable_gems_GEM_FILE} did not set the value of ENABLED_GEMS.\n"
                                "Gem Files should contain set(ENABLED_GEMS ... <list of gem names>)")
        endif()
        set(ly_enable_gems_GEMS ${ENABLED_GEMS})
        set(ENABLED_GEMS ${store_temp}) # restore value of ENABLED_GEMS just in case...
    endif()

    # all the actual work has to be done later.
    foreach(target_name ${ly_enable_gems_TARGETS})
        foreach(variant_name ${ly_enable_gems_VARIANTS})
            set_property(GLOBAL APPEND PROPERTY LY_DELAYED_ENABLE_GEMS "${ly_enable_gems_PROJECT_NAME},${target_name},${variant_name}")
            set_property(GLOBAL APPEND PROPERTY LY_DELAYED_ENABLE_GEMS_"${ly_enable_gems_PROJECT_NAME},${target_name},${variant_name}" ${ly_enable_gems_GEMS})
        endforeach()
    endforeach()
endfunction()

# call this before runtime dependencies are used to add any relevant targets
# saved by the above function
function(ly_enable_gems_delayed)
    get_property(ly_delayed_enable_gems GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS)
    foreach(project_target_variant ${ly_delayed_enable_gems})
        # we expect a colon seperated list of 
        # PROJECT_NAME,target_name,variant_name
        string(REPLACE "," ";" project_target_variant_list "${project_target_variant}")
        list(LENGTH project_target_variant_list project_target_variant_length)
        if(project_target_variant_length EQUAL 0)
            continue()
        endif()
        
        if(NOT project_target_variant_length EQUAL 3)
            message(FATAL_ERROR "Invalid specificaiton of gems, expected 'project','target','variant' and got ${project_target_variant}")
        endif()

        list(POP_BACK project_target_variant_list variant)
        list(POP_BACK project_target_variant_list target)
        list(POP_BACK project_target_variant_list project)

        get_property(gem_dependencies GLOBAL PROPERTY LY_DELAYED_ENABLE_GEMS_"${project_target_variant}")
        if (NOT gem_dependencies)
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
            
            if (TARGET Gem::${gem_name}.${variant})
                ly_add_target_dependencies(
                    ${PREFIX_CLAUSE}
                    TARGETS ${target}
                    DEPENDENT_TARGETS Gem::${gem_name}.${variant}
                )
            elseif(${variant} STREQUAL "Client" AND TARGET Gem::${gem_name})
                # Client can also be 'empty' for backward compatibility
                ly_add_target_dependencies(
                    ${PREFIX_CLAUSE}
                    TARGETS ${target}
                    DEPENDENT_TARGETS Gem::${gem_name}
                )
            endif()
        endforeach()
    endforeach()
endfunction()