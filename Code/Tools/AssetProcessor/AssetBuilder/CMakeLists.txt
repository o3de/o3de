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

ly_add_target(
    NAME AssetBuilder EXECUTABLE
    NAMESPACE AZ
    FILES_CMAKE
        asset_builder_files.cmake
        Platform/${PAL_PLATFORM_NAME}/asset_builder_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Network
            AZ::AzCore
            AZ::AssetBuilderSDK
            AZ::AzToolsFramework
)

# Aggregates all combined AssetBuilders into a single LY_ASSET_BUILDERS #define
get_property(asset_builders GLOBAL PROPERTY LY_ASSET_BUILDERS)
string (REPLACE ";" "," asset_builders "${asset_builders}")
ly_add_source_properties(
    SOURCES AssetBuilderComponent.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES LY_ASSET_BUILDERS="${asset_builders}"
)

if(TARGET AssetBuilder)
    # Adds the AssetBuilder target as a C preprocessor define so that it can be used as a Settings Registry
    # specialization in order to look up the generated .setreg which contains the dependencies
    # specified for the AssetBuilder in the <Project>/Gem/Code/CMakeLists via ly_add_project_dependencies
    ly_add_source_properties(
        SOURCES AssetBuilderApplication.cpp
        PROPERTY COMPILE_DEFINITIONS
        VALUES LY_CMAKE_TARGET="AssetBuilder"
    )
else()
    message(FATAL_ERROR "Cannot set LY_CMAKE_TARGET define to AssetBuilder as the target doesn't exist anymore."
        " Perhaps it has been renamed")
endif()

