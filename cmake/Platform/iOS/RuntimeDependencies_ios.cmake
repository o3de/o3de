#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_set(IOS_FRAMEWORK_TARGET_TYPES MODULE_LIBRARY SHARED_LIBRARY)
ly_set(LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS MODULE_LIBRARY SHARED_LIBRARY EXECUTABLE)

# Function that recursively retrieves all the .framework dependencies of an iOS project.
# There is a generic 'ly_get_runtime_dependencies' function we could use instead of
# this, but because of the hack where game launchers depend on other game launchers
# that function ends up returning dependencies for game projects other than the one
# passed to it (if you have multiple game projects enabled). If the hack is removed,
# we can remove this function and simply call 'ly_get_runtime_dependencies' instead.
function(ios_get_dependencies_recursive ios_DEPENDENCIES ly_TARGET)
    # check to see if this target is a 3rdParty lib that was a downloaded package,
    # and if so, activate it.  This also calls find_package so there is no reason
    # to do so later.
    ly_parse_third_party_dependencies(${ly_TARGET})
    # The above needs to be done before the below early out of this function!
    if(NOT TARGET ${ly_TARGET})
        return() # Nothing to do
    endif()

    # See if we already have dependencies cached.
    get_property(are_dependencies_cached GLOBAL PROPERTY LY_RUNTIME_DEPENDENCIES_${ly_TARGET} SET)
    if(are_dependencies_cached)

        # We already walked through this target
        get_property(cached_dependencies GLOBAL PROPERTY LY_RUNTIME_DEPENDENCIES_${ly_TARGET})
        set(${ios_DEPENDENCIES} ${cached_dependencies} PARENT_SCOPE)
        return()

    endif()

    # Collect all direct dependencies.
    unset(direct_dependencies)
    unset(dependencies)
    get_target_property(dependencies ${ly_TARGET} MANUALLY_ADDED_DEPENDENCIES)
    if(dependencies)
        list(APPEND direct_dependencies ${dependencies})
    endif()
    unset(dependencies)
    get_target_property(dependencies ${ly_TARGET} INTERFACE_LINK_LIBRARIES)
    if(dependencies)
        list(APPEND direct_dependencies ${dependencies})
    endif()
    get_target_property(target_type ${ly_TARGET} TYPE)
    if(NOT target_type MATCHES "INTERFACE")
        unset(dependencies)
        get_target_property(dependencies ${ly_TARGET} LINK_LIBRARIES)
        if(dependencies)
            list(APPEND direct_dependencies ${dependencies})
        endif()
    endif()

    # Iterate over all direct dependencies.
    unset(all_dependencies)
    foreach(direct_dependency ${direct_dependencies})
        if(NOT ${direct_dependency} MATCHES "^::@") # Skip wraping produced when targets are not created in the same directory (https://cmake.org/cmake/help/latest/prop_tgt/LINK_LIBRARIES.html)

            # Get the target type of the direct dependency.
            if(TARGET ${direct_dependency})
                get_target_property(dependency_target_type ${direct_dependency} TYPE)
            else()
                set(dependency_target_type "NON_TARGET")
            endif()

            if(dependency_target_type IN_LIST IOS_FRAMEWORK_TARGET_TYPES)
                # XCode expects target name without namespace
                get_target_property(dependency_name ${direct_dependency} NAME)
                list(APPEND all_dependencies ${dependency_name})
            else()
                get_filename_component(file_extension ${direct_dependency} LAST_EXT)
                if(file_extension MATCHES ".framework")
                    # We want to copy 3rdParty frameworks only. System frameworks should not be embedded.
                    if(NOT ${direct_dependency} MATCHES "^/Applications/Xcode.app")
                        list(APPEND all_dependencies ${direct_dependency})
                    endif()
                endif()
            endif()

            # Recursively look for indirect dependencies, skipping over executables
            # to get around the issue of game projects depending on other game projects.
            if(NOT ${dependency_target_type} STREQUAL "EXECUTABLE")
                unset(indirect_dependencies)
                ios_get_dependencies_recursive(indirect_dependencies ${direct_dependency})
                list(APPEND all_dependencies ${indirect_dependencies})
            endif()

        endif()
    endforeach()

    # Account for imported dependencies.
    get_target_property(is_imported ${ly_TARGET} IMPORTED)
    if(is_imported)
        if(target_type MATCHES "INTERFACE")
            set(imported_property INTERFACE_IMPORTED_LOCATION)
        else()
            set(imported_property IMPORTED_LOCATION)
        endif()
        get_target_property(imported_dependencies ${ly_TARGET} ${imported_property})
        if(imported_dependencies)
            list(APPEND all_dependencies ${imported_dependencies})
        endif()
    endif()

    # Remove duplicate dependencies and return.
    list(REMOVE_DUPLICATES all_dependencies)
    set_property(GLOBAL PROPERTY LY_RUNTIME_DEPENDENCIES_${ly_TARGET} "${all_dependencies}")
    set(${ios_DEPENDENCIES} ${all_dependencies} PARENT_SCOPE)

endfunction()

function(ly_delayed_generate_runtime_dependencies)

    # For each (non-monolithic) game project, find runtime dependencies and tell XCode to embed/sign them
    if(NOT LY_MONOLITHIC_GAME)

        foreach(game_project ${LY_PROJECTS})

            # Recursively get all dependent frameworks for the game project.
            unset(dependencies)
            ios_get_dependencies_recursive(dependencies ${game_project}.GameLauncher)
            if(dependencies)
                set_target_properties(${game_project}.GameLauncher
                    PROPERTIES
                    XCODE_EMBED_FRAMEWORKS "${dependencies}"
                    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY TRUE
                    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
                )
            endif()

        endforeach()

    endif()

    get_property(all_targets GLOBAL PROPERTY LY_ALL_TARGETS)
    unset(test_runner_dependencies)
    foreach(aliased_target IN LISTS all_targets)

        unset(target)
        ly_de_alias_target(${aliased_target} target)

        # Exclude targets that dont produce runtime outputs
        get_target_property(target_type ${target} TYPE)
        if(NOT target_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS)
            continue()
        endif()
        
        file(GENERATE
            OUTPUT ${CMAKE_BINARY_DIR}/runtime_dependencies/$<CONFIG>/${target}.cmake
            CONTENT ""
        )

        if(target_type IN_LIST IOS_FRAMEWORK_TARGET_TYPES)
            list(APPEND test_runner_dependencies ${target})
        endif()
    endforeach()

    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        add_dependencies("AzTestRunner" ${test_runner_dependencies})
        
        # We still need to add indirect dependencies(eg. 3rdParty)
        unset(all_dependencies)
        ios_get_dependencies_recursive(all_dependencies AzTestRunner)

        set_target_properties("AzTestRunner"
            PROPERTIES
            XCODE_EMBED_FRAMEWORKS "${all_dependencies}"
            XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY TRUE
            XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
        )
    endif()

endfunction()