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

function(ly_get_filtered_runtime_dependencies dependencies target)
    
    unset(all_dependencies)
    unset(filtered_dependencies)
    ly_get_runtime_dependencies(all_dependencies ${target})
    
    # ly_get_runtime_dependencies also returns asset files that are
    # associated with a target. However, we only want frameworks(runtime
    # libraries) here which Xcode will embed in the app bundle
    foreach(dependency IN LISTS all_dependencies)
        if (TARGET ${dependency})
            list(APPEND filtered_dependencies ${dependency})
        endif()
    endforeach()

    set(${dependencies} ${filtered_dependencies} PARENT_SCOPE)
endfunction()

function(ly_delayed_generate_runtime_dependencies)

    # For each (non-monolithic) game project, find runtime dependencies and tell XCode to embed/sign them
    if(NOT LY_MONOLITHIC_GAME)

        foreach(game_project ${LY_PROJECTS})

            # Recursively get all dependent frameworks for the game project.
            unset(dependencies)
            ly_get_filtered_runtime_dependencies(dependencies ${game_project}.GameLauncher)
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
        unset(dependencies)
        ly_get_filtered_runtime_dependencies(dependencies AzTestRunner)

        set_target_properties("AzTestRunner"
            PROPERTIES
            XCODE_EMBED_FRAMEWORKS "${dependencies}"
            XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY TRUE
            XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
        )
    endif()

endfunction()