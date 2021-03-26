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

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

include(${pal_dir}/LauncherUnified_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

ly_add_target(
    NAME Launcher.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        launcher_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            ${pal_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzGameFramework
            Legacy::CryCommon
)

ly_add_target(
    NAME Launcher.Game.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        launcher_game_files.cmake
        ${pal_dir}/launcher_game_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzGameFramework
            Legacy::CryCommon
)

if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)

    ly_add_target(
        NAME Launcher.Server.Static STATIC
        NAMESPACE AZ
        FILES_CMAKE
            launcher_server_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzGameFramework
                Legacy::CryCommon
    )

endif()

get_property(LY_PROJECTS_TARGET_NAME GLOBAL PROPERTY LY_PROJECTS_TARGET_NAME)
foreach(project_name project_path IN ZIP_LISTS LY_PROJECTS_TARGET_NAME LY_PROJECTS)
    # Computes the realpath to the project
    # If the project_path is relative, it is evaluated relative to the ${LY_ROOT_FOLDER}
    # Otherwise the the absolute project_path is returned with symlinks resolved
    file(REAL_PATH ${project_path} project_real_path BASE_DIRECTORY ${LY_ROOT_FOLDER})
    ################################################################################
    # Monolithic game
    ################################################################################
    if(LY_MONOLITHIC_GAME)

        # In the monolithic case, we need to register the gem modules, to do so we will generate a StaticModules.inl
        # file from StaticModules.in

        get_property(game_gem_dependencies GLOBAL PROPERTY LY_DELAYED_DEPENDENCIES_${project_name}.GameLauncher)
        
        unset(extern_module_declarations)
        unset(module_invocations)

        foreach(game_gem_dependency ${game_gem_dependencies})
            # To match the convention on how gems targets vs gem modules are named, we remove the "Gem::" from prefix 
            # and remove the ".Static" from the suffix
            string(REGEX REPLACE "^Gem::" "Gem_" game_gem_dependency ${game_gem_dependency})
            string(REGEX REPLACE "^Project::" "Project_" game_gem_dependency ${game_gem_dependency})
            # Replace "." with "_"
            string(REPLACE "." "_" game_gem_dependency ${game_gem_dependency})

            string(APPEND extern_module_declarations "extern \"C\" AZ::Module* CreateModuleClass_${game_gem_dependency}();\n")
            string(APPEND module_invocations "    modulesOut.push_back(CreateModuleClass_${game_gem_dependency}());\n")

        endforeach()

        configure_file(StaticModules.in
            ${CMAKE_CURRENT_BINARY_DIR}/${project_name}.GameLauncher/Includes/StaticModules.inl
        )

        set(game_build_dependencies 
            ${game_gem_dependencies}
            Legacy::CrySystem
            Legacy::CryFont
            Legacy::Cry3DEngine
        )

        if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
            get_property(server_gem_dependencies GLOBAL PROPERTY LY_DELAYED_DEPENDENCIES_${project_name}.ServerLauncher)
        
            unset(extern_module_declarations)
            unset(module_invocations)

            foreach(server_gem_dependency ${server_gem_dependencies})
                # To match the convention on how gems targets vs gem modules are named, we remove the "Gem::" from prefix 
                # and remove the ".Static" from the suffix
                string(REGEX REPLACE "^Gem::" "Gem_" server_gem_dependency ${server_gem_dependency})
                string(REGEX REPLACE "^Project::" "Project_" server_gem_dependency ${server_gem_dependency})
                # Replace "." with "_"
                string(REPLACE "." "_" server_gem_dependency ${server_gem_dependency})

                string(APPEND extern_module_declarations "extern \"C\" AZ::Module* CreateModuleClass_${server_gem_dependency}();\n")
                string(APPEND module_invocations "    modulesOut.push_back(CreateModuleClass_${server_gem_dependency}());\n")

            endforeach()

            configure_file(StaticModules.in
                ${CMAKE_CURRENT_BINARY_DIR}/${project_name}.ServerLauncher/Includes/StaticModules.inl
            )

            set(server_build_dependencies 
                ${game_gem_dependencies}
                Legacy::CrySystem
                Legacy::CryFont
                Legacy::Cry3DEngine
            )
        endif()

    else()

        set(game_runtime_dependencies
            Legacy::CrySystem
            Legacy::CryFont
            Legacy::Cry3DEngine
        )
        if(PAL_TRAIT_BUILD_SERVER_SUPPORTED AND NOT LY_MONOLITHIC_GAME)  # Only Atom is supported in monolithic builds
            set(server_runtime_dependencies
                Legacy::CryRenderNULL
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
            launcher_project_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/launcher_project_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                # Adds the name of the project/game
                LY_PROJECT_NAME="${project_name}"
                # Adds the project path supplied to CMake during configuration
                # This is used as a fallback to launch the AssetProcessor
                LY_PROJECT_CMAKE_PATH="${project_path}"
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
    )

    ################################################################################
    # Server
    ################################################################################
    if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)

        get_property(server_projects GLOBAL PROPERTY LY_LAUNCHER_SERVER_PROJECTS)
        if(${project_name} IN_LIST server_projects)

            ly_add_target(
                NAME ${project_name}.ServerLauncher APPLICATION
                NAMESPACE AZ
                FILES_CMAKE
                    launcher_project_files.cmake
                PLATFORM_INCLUDE_FILES
                    ${pal_dir}/launcher_project_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
                COMPILE_DEFINITIONS
                    PRIVATE
                        # Adds the name of the project/game
                        LY_PROJECT_NAME="${project_name}"
                        # Adds the project path supplied to CMake during configuration
                        # This is used as a fallback to launch the AssetProcessor
                        LY_PROJECT_CMAKE_PATH="${project_path}"
                        # Adds the ${project_name}_ServerLauncher target as a define so for the Settings Registry to use
                        # when loading .setreg file specializations
                        # This is needed so that only gems for the project server launcher are loaded
                        LY_CMAKE_TARGET="${project_name}_ServerLauncher"
                INCLUDE_DIRECTORIES
                    PRIVATE
                        .
                        ${CMAKE_CURRENT_BINARY_DIR}/${project_name}.ServerLauncher/Includes # required for StaticModules.inl
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::Launcher.Static
                        AZ::Launcher.Server.Static
                        ${server_build_dependencies}
                RUNTIME_DEPENDENCIES
                    ${server_runtime_dependencies}
            )
            # Needs to be set manually after ly_add_target to prevent the default location overriding it
            set_target_properties(${project_name}.ServerLauncher
                PROPERTIES 
                    FOLDER ${project_name}
            )
        endif()

    endif()

endforeach()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    
    ly_add_target(
        NAME Launcher.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            launcher_test_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                LY_CMAKE_TARGET="Launcher_Tests"
        INCLUDE_DIRECTORIES
            PRIVATE
                . 
                ${pal_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::Launcher.Static
    )
    ly_add_googletest(
        NAME AZ::Launcher.Tests
    )

endif()

