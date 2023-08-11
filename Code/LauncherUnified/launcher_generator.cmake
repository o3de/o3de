#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This will be used to turn on "PerMonitor" DPI scaling support. (Currently there is no way in CMake to specify "PerMonitorV2")
set(O3DE_DPI_AWARENESS "PerMonitor")

set_property(GLOBAL PROPERTY SERVER_LAUNCHER_TYPES ServerLauncher HeadlessServerLauncher)

set_property(GLOBAL PROPERTY LAUNCHER_UNIFIED_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(SERVER_LAUNCHER_TYPES ServerLauncher HeadlessServerLauncher)

set(SERVER_LAUNCHERTYPE_ServerLauncher APPLICATION)
set(SERVER_BUILD_DEPENDENCIES_ServerLauncher AZ::Launcher.Static AZ::AzGameFramework AZ::Launcher.Server.Static)
set(SERVER_VARIANT_ServerLauncher Servers)

set(SERVER_LAUNCHERTYPE_HeadlessServerLauncher EXECUTABLE)
set(SERVER_BUILD_DEPENDENCIES_HeadlessServerLauncher AZ::Launcher.Headless.Static AZ::AzGameFramework.Headless AZ::Launcher.Server.Static)
set(SERVER_VARIANT_HeadlessServerLauncher HeadlessServers)

# Launcher targets for a project need to be generated when configuring a project.
# When building the engine source, this file will be included by LauncherUnified's CMakeLists.txt
# When using an installed engine, this file will be included by the FindLauncherGenerator.cmake script
get_property(SERVER_LAUNCHER_TYPES GLOBAL PROPERTY SERVER_LAUNCHER_TYPES)
get_property(O3DE_PROJECTS_NAME GLOBAL PROPERTY O3DE_PROJECTS_NAME)
foreach(project_name project_path IN ZIP_LISTS O3DE_PROJECTS_NAME LY_PROJECTS)

    # Computes the realpath to the project
    # If the project_path is relative, it is evaluated relative to the ${LY_ROOT_FOLDER}
    # Otherwise the the absolute project_path is returned with symlinks resolved
    file(REAL_PATH ${project_path} project_real_path BASE_DIRECTORY ${LY_ROOT_FOLDER})

    ################################################################################
    # Assets
    ################################################################################
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        add_custom_target(${project_name}.Assets
            COMMENT "Processing ${project_name} assets..."
            COMMAND "${CMAKE_COMMAND}"
                -DLY_LOCK_FILE=$<GENEX_EVAL:$<TARGET_FILE_DIR:AZ::AssetProcessorBatch>>/project_assets.lock
                -P ${LY_ROOT_FOLDER}/cmake/CommandExecution.cmake
                    EXEC_COMMAND $<GENEX_EVAL:$<TARGET_FILE:AZ::AssetProcessorBatch>>
                        --zeroAnalysisMode
                        --project-path=${project_real_path}
                        --platforms=${LY_ASSET_DEPLOY_ASSET_TYPE}
        )
        set_target_properties(${project_name}.Assets
            PROPERTIES
                EXCLUDE_FROM_ALL TRUE
                FOLDER ${project_name}
        )
    endif()

    ################################################################################
    # Monolithic game
    ################################################################################
    if(LY_MONOLITHIC_GAME)

        # In the monolithic case, we need to register the gem modules, to do so we will generate a StaticModules.inl
        # file from StaticModules.in
        set_property(GLOBAL APPEND PROPERTY LY_STATIC_MODULE_PROJECTS_NAME ${project_name})
        get_property(game_gem_dependencies GLOBAL PROPERTY LY_DELAYED_DEPENDENCIES_${project_name}.GameLauncher)

        set(game_build_dependencies
            ${game_gem_dependencies}
            Legacy::CrySystem
        )

        if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
            foreach(server_launcher_type ${SERVER_LAUNCHER_TYPES})
                get_property(server_gem_dependencies GLOBAL PROPERTY LY_DELAYED_DEPENDENCIES_${project_name}.${server_launcher_type})
                list(APPEND SERVER_BUILD_DEPENDENCIES_${server_launcher_type}
                    ${server_gem_dependencies}
                    Legacy::CrySystem
                )
            endforeach()
        endif()

        if(PAL_TRAIT_BUILD_UNIFIED_SUPPORTED)
            get_property(unified_gem_dependencies GLOBAL PROPERTY LY_DELAYED_DEPENDENCIES_${project_name}.UnifiedLauncher)

            set(unified_build_dependencies
                ${unified_gem_dependencies}
                Legacy::CrySystem
            )
        endif()

    else()

        set(game_runtime_dependencies
            Legacy::CrySystem
        )

        if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
            set(server_runtime_dependencies
                Legacy::CrySystem
            )
        endif()

        if(PAL_TRAIT_BUILD_UNIFIED_SUPPORTED)
            set(unified_runtime_dependencies
                Legacy::CrySystem
            )
        endif()

    endif()

    ################################################################################
    # Game
    ################################################################################
    ly_add_target(
        NAME ${project_name}.GameLauncher ${PAL_TRAIT_LAUNCHERUNIFIED_LAUNCHER_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            ${CMAKE_CURRENT_LIST_DIR}/launcher_project_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/launcher_project_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                # Adds the name of the project/game
                LY_PROJECT_NAME="${project_name}"
                # Adds the ${project_name}_GameLauncher target as a define so for the Settings Registry to use
                # when loading .setreg file specializations
                # This is needed so that only gems for the project game launcher are loaded
                LY_CMAKE_TARGET="${project_name}_GameLauncher"
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                ${CMAKE_CURRENT_BINARY_DIR}/${project_name}.GameLauncher/Includes # required for StaticModules.inl
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::Launcher.Static
                AZ::Launcher.Game.Static
                ${game_build_dependencies}
        RUNTIME_DEPENDENCIES
            ${game_runtime_dependencies}
    )
    # Needs to be set manually after ly_add_target to prevent the default location overriding it
    set_target_properties(${project_name}.GameLauncher
        PROPERTIES
            FOLDER ${project_name}
            LY_PROJECT_NAME ${project_name}
    )

    # Turn on DPI scaling support.
    set_property(TARGET ${project_name}.GameLauncher APPEND PROPERTY VS_DPI_AWARE ${O3DE_DPI_AWARENESS})

    if(LY_DEFAULT_PROJECT_PATH)
        if (TARGET ${project_name})
            get_target_property(project_game_launcher_additional_args ${project_name} GAMELAUNCHER_ADDITIONAL_VS_DEBUGGER_COMMAND_ARGUMENTS)
            if (project_game_launcher_additional_args)
                # Avoid pushing param-NOTFOUND into the argument in case this property wasn't found
                set(additional_game_vs_debugger_args "${project_game_launcher_additional_args}")
            endif()
        endif()

        set_property(TARGET ${project_name}.GameLauncher APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS
            "--project-path=\"${LY_DEFAULT_PROJECT_PATH}\" ${additional_game_vs_debugger_args}")
    endif()

    # Associate the Clients Gem Variant with each projects GameLauncher
    ly_set_gem_variant_to_load(TARGETS ${project_name}.GameLauncher VARIANTS Clients)

    ################################################################################
    # Server
    ################################################################################
    if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)

        foreach(server_launcher_type ${SERVER_LAUNCHER_TYPES})

            ly_add_target(
                NAME ${project_name}.${server_launcher_type} ${SERVER_LAUNCHERTYPE_${server_launcher_type}}
                NAMESPACE AZ
                FILES_CMAKE
                    ${CMAKE_CURRENT_LIST_DIR}/launcher_project_files.cmake
                PLATFORM_INCLUDE_FILES
                    ${pal_dir}/launcher_project_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
                COMPILE_DEFINITIONS
                    PRIVATE
                        # Adds the name of the project/game
                        LY_PROJECT_NAME="${project_name}"
                        # Adds the ${project_name}_${server_launcher_type} target as a define so for the Settings Registry to use
                        # when loading .setreg file specializations
                        # This is needed so that only gems for the project server launcher are loaded
                        LY_CMAKE_TARGET="${project_name}_${server_launcher_type}"

                INCLUDE_DIRECTORIES
                    PRIVATE
                        .
                        ${CMAKE_CURRENT_BINARY_DIR}/${project_name}.${server_launcher_type}/Includes # required for StaticModules.inl
                BUILD_DEPENDENCIES
                    PRIVATE
                        ${SERVER_BUILD_DEPENDENCIES_${server_launcher_type}}
                RUNTIME_DEPENDENCIES
                    ${server_runtime_dependencies}
            )
            # Needs to be set manually after ly_add_target to prevent the default location overriding it
            set_target_properties(${project_name}.${server_launcher_type}
                PROPERTIES
                    FOLDER ${project_name}
                    LY_PROJECT_NAME ${project_name}
            )

            # Turn on DPI scaling support.
            set_property(TARGET ${project_name}.${server_launcher_type} APPEND PROPERTY VS_DPI_AWARE ${O3DE_DPI_AWARENESS})

            if(LY_DEFAULT_PROJECT_PATH)
                if (TARGET ${project_name})
                    get_target_property(project_server_launcher_additional_args ${project_name} SERVERLAUNCHER_ADDITIONAL_VS_DEBUGGER_COMMAND_ARGUMENTS)
                    if (project_server_launcher_additional_args)
                        # Avoid pushing param-NOTFOUND into the argument in case this property wasn't found
                        set(additional_server_vs_debugger_args "${project_server_launcher_additional_args}")
                    endif()
                endif()

                set_property(TARGET ${project_name}.${server_launcher_type} APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS
                    "--project-path=\"${LY_DEFAULT_PROJECT_PATH}\" ${additional_server_vs_debugger_args}")
            endif()

            # Associate the Servers Gem Variant with each projects ServerLauncher
            ly_set_gem_variant_to_load(TARGETS ${project_name}.${server_launcher_type} VARIANTS ${SERVER_VARIANT_${server_launcher_type}})

            # If the Editor is getting built, it should rebuild the ServerLauncher as well. The Editor's game mode will attempt
            # to launch it, and things can break if the two are out of sync.
            if(PAL_TRAIT_BUILD_HOST_TOOLS)
                ly_add_dependencies(Editor ${project_name}.${server_launcher_type})
            endif()
        endforeach()
    endif()

    ################################################################################
    # Unified
    ################################################################################
    if(PAL_TRAIT_BUILD_UNIFIED_SUPPORTED)
        ly_add_target(
            NAME ${project_name}.UnifiedLauncher APPLICATION
            NAMESPACE AZ
            FILES_CMAKE
                ${CMAKE_CURRENT_LIST_DIR}/launcher_project_files.cmake
            PLATFORM_INCLUDE_FILES
                ${pal_dir}/launcher_project_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            COMPILE_DEFINITIONS
                PRIVATE
                    # Adds the name of the project/game
                    LY_PROJECT_NAME="${project_name}"
                    # Adds the ${project_name}_UnifiedLauncher target as a define so for the Settings Registry to use
                    # when loading .setreg file specializations
                    # This is needed so that only gems for the project unified launcher are loaded
                    LY_CMAKE_TARGET="${project_name}_UnifiedLauncher"
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    ${CMAKE_CURRENT_BINARY_DIR}/${project_name}.UnifiedLauncher/Includes # required for StaticModules.inl
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::Launcher.Static
                    AZ::Launcher.Unified.Static
                    ${unified_build_dependencies}
            RUNTIME_DEPENDENCIES
                ${unified_runtime_dependencies}
        )
        # Needs to be set manually after ly_add_target to prevent the default location overriding it
        set_target_properties(${project_name}.UnifiedLauncher
            PROPERTIES
                FOLDER ${project_name}
                LY_PROJECT_NAME ${project_name}
        )

        # Turn on DPI scaling support.
        set_property(TARGET ${project_name}.UnifiedLauncher APPEND PROPERTY VS_DPI_AWARE ${O3DE_DPI_AWARENESS})

        if(LY_DEFAULT_PROJECT_PATH)
            if (TARGET ${project_name})
                get_target_property(project_unified_launcher_additional_args ${project_name} UNIFIEDLAUNCHER_ADDITIONAL_VS_DEBUGGER_COMMAND_ARGUMENTS)
                if (project_unified_launcher_additional_args)
                    # Avoid pushing param-NOTFOUND into the argument in case this property wasn't found
                    set(additional_unified_vs_debugger_args "${project_unified_launcher_additional_args}")
                endif()
            endif()

            set_property(TARGET ${project_name}.UnifiedLauncher APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS
                "--project-path=\"${LY_DEFAULT_PROJECT_PATH}\" ${additional_unified_vs_debugger_args}")
        endif()

        # Associate the Unified Gem Variant with each projects UnfiedLauncher
        ly_set_gem_variant_to_load(TARGETS ${project_name}.UnifiedLauncher VARIANTS Unified)
    endif()
endforeach()

#! Defer generation of the StaticModules.inl file needed in monolithic builds until after all the CMake targets are known
#  This is that the GEM_MODULE target runtime dependencies can be parsed to discover the list of dependent modules
#  to load
function(ly_delayed_generate_static_modules_inl)

    get_property(SERVER_LAUNCHER_TYPES GLOBAL PROPERTY SERVER_LAUNCHER_TYPES)
    if(LY_MONOLITHIC_GAME)
        get_property(launcher_unified_binary_dir GLOBAL PROPERTY LAUNCHER_UNIFIED_BINARY_DIR)
        get_property(project_names GLOBAL PROPERTY LY_STATIC_MODULE_PROJECTS_NAME)
        foreach(project_name ${project_names})

            unset(extern_module_declarations)
            unset(module_invocations)

            unset(all_game_gem_dependencies)
            o3de_get_gem_load_dependencies(
                GEM_LOAD_DEPENDENCIES_VAR all_game_gem_dependencies
                TARGET "${project_name}.GameLauncher")

            foreach(game_gem_dependency ${all_game_gem_dependencies})
                # Sometimes, a gem's Client variant may be an interface library
                # which depends on multiple gem targets. The interface libraries
                # should be skipped; the real dependencies of the interface will be processed
                if(TARGET ${game_gem_dependency})
                    get_target_property(target_type ${game_gem_dependency} TYPE)
                    if(${target_type} STREQUAL "INTERFACE_LIBRARY")
                        continue()
                    endif()
                endif()

                # To match the convention on how gems targets vs gem modules are named,
                # we remove the ".Static" from the suffix
                # Replace "." with "_"
                string(REPLACE "." "_" game_gem_dependency ${game_gem_dependency})

                string(APPEND extern_module_declarations "extern \"C\" AZ::Module* CreateModuleClass_Gem_${game_gem_dependency}();\n")
                string(APPEND module_invocations "    modulesOut.push_back(CreateModuleClass_Gem_${game_gem_dependency}());\n")

            endforeach()

            configure_file(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/StaticModules.in
                ${launcher_unified_binary_dir}/${project_name}.GameLauncher/Includes/StaticModules.inl
            )

            ly_target_link_libraries(${project_name}.GameLauncher PRIVATE ${all_game_gem_dependencies})
            if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
                foreach(server_launcher_type ${SERVER_LAUNCHER_TYPES})

                    unset(extern_module_declarations)
                    unset(module_invocations)

                    unset(all_server_gem_dependencies)

                    o3de_get_gem_load_dependencies(
                        GEM_LOAD_DEPENDENCIES_VAR all_server_gem_dependencies
                        TARGET "${project_name}.${server_launcher_type}")
                    foreach(server_gem_dependency ${all_server_gem_dependencies})
                        # Skip interface libraries
                        if(TARGET ${server_gem_dependency})
                            get_target_property(target_type ${server_gem_dependency} TYPE)
                            if(${target_type} STREQUAL "INTERFACE_LIBRARY")
                                continue()
                            endif()
                        endif()

                        # Replace "." with "_"
                        string(REPLACE "." "_" server_gem_dependency ${server_gem_dependency})

                        string(APPEND extern_module_declarations "extern \"C\" AZ::Module* CreateModuleClass_Gem_${server_gem_dependency}();\n")
                        string(APPEND module_invocations "    modulesOut.push_back(CreateModuleClass_Gem_${server_gem_dependency}());\n")

                    endforeach()

                    configure_file(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/StaticModules.in
                        ${launcher_unified_binary_dir}/${project_name}.${server_launcher_type}/Includes/StaticModules.inl
                    )

                    ly_target_link_libraries(${project_name}.${server_launcher_type} PRIVATE ${all_server_gem_dependencies})
                endforeach()
            endif()

            if(PAL_TRAIT_BUILD_UNIFIED_SUPPORTED)

                unset(extern_module_declarations)
                unset(module_invocations)

                unset(all_unified_gem_dependencies)
                o3de_get_gem_load_dependencies(
                    GEM_LOAD_DEPENDENCIES_VAR all_unified_gem_dependencies
                    TARGET "${project_name}.UnifiedLauncher")
                foreach(unified_gem_dependency ${all_unified_gem_dependencies})
                    # Skip interface libraries
                    if(TARGET ${unified_gem_dependency})
                        get_target_property(target_type ${unified_gem_dependency} TYPE)
                        if(${target_type} STREQUAL "INTERFACE_LIBRARY")
                            continue()
                        endif()
                    endif()

                    # Replace "." with "_"
                    string(REPLACE "." "_" unified_gem_dependency ${unified_gem_dependency})

                    string(APPEND extern_module_declarations "extern \"C\" AZ::Module* CreateModuleClass_Gem_${unified_gem_dependency}();\n")
                    string(APPEND module_invocations "    modulesOut.push_back(CreateModuleClass_Gem_${unified_gem_dependency}());\n")

                endforeach()

                configure_file(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/StaticModules.in
                    ${launcher_unified_binary_dir}/${project_name}.UnifiedLauncher/Includes/StaticModules.inl
                )

                ly_target_link_libraries(${project_name}.UnifiedLauncher PRIVATE ${all_unified_gem_dependencies})
            endif()
        endforeach()
    endif()
endfunction()
