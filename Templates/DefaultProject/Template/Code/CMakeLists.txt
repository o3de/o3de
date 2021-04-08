# {BEGIN_LICENSE}
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# {END_LICENSE}

# Currently we are in the ${Name}/Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here : ${Name}/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/${Name}/Code
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this project
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Now that we have loaded our project traits for this platform, see if this project is even supported on this platform.
# If its not supported we just return after including the unsupported.
if(NOT PAL_TRAIT_${NameUpper}_SUPPORTED)
    return()
endif()

# We are on a supported platform, so add the ${Name} target
# Note: We include the common files and the platform specific files which are set in ${NameLower}_files.cmake and
# in ${pal_dir}/${NameLower}_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME ${Name}.Static STATIC
    NAMESPACE Project
    FILES_CMAKE
        ${NameLower}_files.cmake
        ${pal_dir}/${NameLower}_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzGameFramework
            Gem::Atom_AtomBridge.Static
)

ly_add_target(
    NAME ${Name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Project
    FILES_CMAKE
        ${NameLower}_shared_files.cmake
        ${pal_dir}/${NameLower}_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Project::${Name}.Static
            AZ::AzCore
)

################################################################################
# Gem dependencies
################################################################################
ly_add_project_dependencies(
    PROJECT_NAME
        ${Name}
    TARGETS 
        ${Name}.GameLauncher
    DEPENDENCIES_FILES
        runtime_dependencies.cmake
        ${pal_dir}/${PAL_PLATFORM_NAME_LOWERCASE}_runtime_dependencies.cmake
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_project_dependencies(
        PROJECT_NAME
            ${Name}
        TARGETS
            AssetBuilder
            AssetProcessor
            AssetProcessorBatch
            Editor
        DEPENDENCIES_FILES
            tool_dependencies.cmake
            ${pal_dir}/${PAL_PLATFORM_NAME_LOWERCASE}_tool_dependencies.cmake
    )
endif()

if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
    ly_add_project_dependencies(
        PROJECT_NAME
            ${Name}
        TARGETS
            ${Name}.ServerLauncher
        DEPENDENCIES_FILES
            server_dependencies.cmake
            ${pal_dir}/${PAL_PLATFORM_NAME_LOWERCASE}_server_dependencies.cmake
    )
    set_property(GLOBAL APPEND PROPERTY LY_LAUNCHER_SERVER_PROJECTS ${Name})

endif()
