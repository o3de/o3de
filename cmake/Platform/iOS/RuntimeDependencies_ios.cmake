#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_set(IOS_FRAMEWORK_TARGET_TYPES MODULE_LIBRARY SHARED_LIBRARY)
ly_set(LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS MODULE_LIBRARY SHARED_LIBRARY EXECUTABLE)

include(cmake/Platform/Common/RuntimeDependencies_common.cmake)

function(o3de_get_filtered_dependencies_for_target dependencies target)
    
    unset(filtered_dependencies)
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
    
    # o3de_get_dependencies_for_target also returns asset files that are
    # associated with a target. However, we only want frameworks(runtime
    # libraries) here which Xcode will embed in the app bundle
    # Those dependenices come from the LINK_DEPENDENCIES for the target
    foreach(dependency IN LISTS target_link_dependencies)
        if (TARGET ${dependency})
            # Imported libraries are used as alias to other targets, so
            # only non imported dependencies will be appended.
            get_property(dependency_is_imported TARGET ${dependency} PROPERTY IMPORTED)
            if (NOT dependency_is_imported)
                list(APPEND filtered_dependencies ${dependency})
            endif()

            # Recursively resolve alias' dependencies.
            # Retrieve the de-aliased targets using the target dependency as key into the O3DE_ALIASED_TARGETS_* "dict"
            get_property(dealiased_targets GLOBAL PROPERTY O3DE_ALIASED_TARGETS_${dependency})
            if (dealiased_targets)
                foreach(dealiased_target ${dealiased_targets})
                    unset(dealiased_dependencies)
                    ly_get_filtered_runtime_dependencies(dealiased_dependencies ${dealiased_target})
                    list(APPEND filtered_dependencies ${dealiased_dependencies})
                endforeach()
            endif()
        endif()
    endforeach()

    list(REMOVE_DUPLICATES filtered_dependencies)
    set(${dependencies} ${filtered_dependencies} PARENT_SCOPE)
endfunction()

function(ly_delayed_generate_runtime_dependencies)

    # For each (non-monolithic) game project, find runtime dependencies and tell XCode to embed/sign them
    if(NOT LY_MONOLITHIC_GAME)
        get_property(project_names GLOBAL PROPERTY O3DE_PROJECTS_NAME)
        foreach(project_name IN LISTS project_names)

            # Recursively get all dependent frameworks for the game project.
            unset(dependencies)
            o3de_get_filtered_dependencies_for_target(dependencies ${project_name}.GameLauncher)

            if(dependencies)
                set_target_properties(${project_name}.GameLauncher
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
        unset(dependencies)
        o3de_get_filtered_dependencies_for_target(dependencies AzTestRunner)

        if(dependencies)
            set_target_properties("AzTestRunner"
                PROPERTIES
                XCODE_EMBED_FRAMEWORKS "${dependencies}"
                XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY TRUE
                XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
            )
        endif()
    endif()

endfunction()
