# {BEGIN_LICENSE}
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
# {END_LICENSE}

#! Adds the --project-path argument to the VS IDE debugger command arguments
function(add_vs_debugger_arguments)
    # Inject the project root into the --project-path argument into the Visual Studio Debugger arguments by defaults
    list(APPEND app_targets ${Name}.GameLauncher ${Name}.ServerLauncher)
    list(APPEND app_targets AssetBuilder AssetProcessor AssetProcessorBatch Editor)
    foreach(app_target IN LISTS app_targets)
        if (TARGET ${app_target})
            set_property(TARGET ${app_target} APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "--project-path=\"${CMAKE_CURRENT_LIST_DIR}\"")
        endif()
    endforeach()
endfunction()

set(o3de_project_path ${CMAKE_CURRENT_LIST_DIR})
set(o3de_project_json ${o3de_project_path}/project.json)

if(NOT PROJECT_NAME)
    cmake_minimum_required(VERSION 3.19)
    project(${Name}
        LANGUAGES C CXX
        VERSION 1.0.0.0
    )

    # set this project as the only project
    set(LY_PROJECTS ${CMAKE_CURRENT_LIST_DIR})

    # o3de manifest
    include(o3de_manifest.cmake)

    ################################################################################
    # Set the engine_path and resolve this engines restricted path if it has one
    ################################################################################
    o3de_engine_path(${o3de_project_json} o3de_engine_path)
    o3de_project_name(${o3de_project_json} o3de_project_name)
    o3de_restricted_path(${o3de_project_json} o3de_project_restricted_path)
    message(STATUS "O3DE Project Name: ${o3de_project_name}")
    message(STATUS "O3DE Project Path: ${o3de_project_path}")
    if(${o3de_project_restricted_path})
        message(STATUS "O3DE Project Restricted Path: ${o3de_project_restricted_path}")
    endif()

    # add the engines cmake folder to the CMAKE_MODULE_PATH
    list(APPEND CMAKE_MODULE_PATH "${o3de_engine_path}/cmake")

    # add subdirectory on the engine path for this project
    add_subdirectory(${o3de_engine_path} o3de)

    # add this --project-path argumens to visual studio debugger
    add_vs_debugger_arguments()

else()
    ######################################################
    # the engine is calling add sub_directory() on us
    ######################################################
    o3de_project_name(${o3de_project_json} o3de_project_name)
    o3de_restricted_path(${o3de_project_json} o3de_project_restricted_path)

    # Currently we are in the TestDP folder: ${CMAKE_CURRENT_LIST_DIR}
    # Get the platform specific folder ${pal_dir} for the folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
    # Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
    #       in which case it will see if that platform is present here or in the restricted folder.
    #       i.e. It could here: TestDP/Platform/<platorm_name>  or
    #            <restricted_folder>/<platform_name>/TestDP
    ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_project_restricted_path} ${o3de_project_path} ${o3de_project_name})

    # Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
    # project cmake for this platform.
    include(${pal_dir}/${PAL_PLATFORM_NAME_LOWERCASE}_project.cmake)

    # Add the project_name to global LY_PROJECTS_TARGET_NAME property
    set_property(GLOBAL APPEND PROPERTY LY_PROJECTS_TARGET_NAME ${o3de_project_name})

    add_subdirectory(Code)
endif()
