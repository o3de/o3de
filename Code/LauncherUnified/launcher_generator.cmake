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

# Launcher targets for a project need to be generated when configuring a project.
# When building the engine source, this file will be included by LauncherUnified's CMakeLists.txt
# When using an installed engine, this file will be included by the FindLauncherGenerator.cmake script
get_property(LY_PROJECTS_TARGET_NAME GLOBAL PROPERTY LY_PROJECTS_TARGET_NAME)
foreach(project_name project_path IN ZIP_LISTS LY_PROJECTS_TARGET_NAME LY_PROJECTS)
    # Computes the realpath to the project
    # If the project_path is relative, it is evaluated relative to the ${LY_ROOT_FOLDER}
    # Otherwise the the absolute project_path is returned with symlinks resolved
    file(REAL_PATH ${project_path} project_real_path BASE_DIRECTORY ${LY_ROOT_FOLDER})
    if(NOT project_name)
        if(NOT EXISTS ${project_real_path}/project.json)
            message(FATAL_ERROR "The specified project path of ${project_real_path} does not contain a project.json file")
        else()
            # Add the project_name to global LY_PROJECTS_TARGET_NAME property
            file(READ "${project_real_path}/project.json" project_json)
            string(JSON project_name ERROR_VARIABLE json_error GET ${project_json} "project_name")
            if(json_error)
                message(FATAL_ERROR "There is an error reading the \"project_name\" key from the '${project_real_path}/project.json' file: ${json_error}")
            endif()
            message(WARNING "The project located at path ${project_real_path} has a valid \"project name\" of '${project_name}' read from it's project.json file."
                " This indicates that the ${project_real_path}/CMakeLists.txt is not properly appending the \"project name\" "
                "to the LY_PROJECTS_TARGET_NAME global property. Other configuration errors might occur")
        endif()
    endif()
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
            ${CMAKE_CURRENT_LIST_DIR}/launcher_project_files.cmake
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
                    ${CMAKE_CURRENT_LIST_DIR}/launcher_project_files.cmake
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
