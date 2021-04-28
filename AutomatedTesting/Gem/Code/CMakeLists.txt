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
    NAME AutomatedTesting ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        automatedtesting_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::Atom_AtomBridge.Static
)

################################################################################
# Gem dependencies
################################################################################

# The GameLauncher uses "Client" gem variants:
ly_enable_gems(PROJECT_NAME AutomatedTesting GEM_FILE enabled_gems.cmake
    TARGETS AutomatedTesting.GameLauncher
    VARIANTS Clients)

# The Editor uses Tools gem variants:
ly_enable_gems(
    PROJECT_NAME AutomatedTesting GEM_FILE enabled_gems.cmake
    TARGETS Editor
    VARIANTS Tools)

# The pipeline tools use Builders gem variants:
ly_enable_gems(
    PROJECT_NAME AutomatedTesting GEM_FILE enabled_gems.cmake
    TARGETS AssetBuilder AssetProcessor AssetProcessorBatch
    VARIANTS Builders)

# old system (remove when all gems are ported to the new system above)

ly_add_project_dependencies(
    PROJECT_NAME
        AutomatedTesting
    TARGETS 
        AutomatedTesting.GameLauncher
    DEPENDENCIES_FILES
        runtime_dependencies.cmake
        ${pal_dir}/runtime_dependencies.cmake
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_project_dependencies(
        PROJECT_NAME
            AutomatedTesting
        TARGETS
            AssetBuilder
            AssetProcessor
            AssetProcessorBatch
            Editor
        DEPENDENCIES_FILES
            tool_dependencies.cmake
            ${pal_dir}/tool_dependencies.cmake
    )
endif()
