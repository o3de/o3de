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

ly_add_target(
    NAME CMakeTestbed ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        cmaketestbed_files.cmake
        ${pal_dir}/cmaketestbed_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzGameFramework
)

################################################################################
# Gem dependencies
################################################################################
ly_add_project_dependencies(
    PROJECT_NAME
        CMakeTestbed
    TARGETS 
        CMakeTestbed.GameLauncher
    DEPENDENCIES_FILES
        runtime_dependencies.cmake
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_project_dependencies(
        PROJECT_NAME
            CMakeTestbed
        TARGETS
            AssetBuilder
            AssetProcessor
            AssetProcessorBatch
            Editor
        DEPENDENCIES_FILES
            tool_dependencies.cmake
    )
endif()

if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
    ly_add_project_dependencies(
        PROJECT_NAME
            CMakeTestbed
        TARGETS
            CMakeTestbed.ServerLauncher
        DEPENDENCIES_FILES
            runtime_dependencies.cmake
    )
    set_property(GLOBAL APPEND PROPERTY LY_LAUNCHER_SERVER_PROJECTS CMakeTestbed)

endif()
