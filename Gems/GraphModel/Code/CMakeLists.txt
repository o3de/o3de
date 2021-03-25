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

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME GraphModel.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            graphmodel_editor_static_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                GRAPHMODEL_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                Gem::GraphCanvasWidgets
    )
    ly_add_target(
        NAME GraphModel.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.GraphModel.Editor.0844f64a3acf4f5abf3a535dc9b63bc9.v0.1.0
        FILES_CMAKE
            graphmodel_editor_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                GRAPHMODEL_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                Gem::GraphCanvasWidgets
                Gem::GraphModel.Editor.Static
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME GraphModel.Editor.Tests MODULE
            NAMESPACE Gem
            FILES_CMAKE
                graphmodel_tests_editor_files.cmake
            COMPILE_DEFINITIONS
                PRIVATE
                    GRAPHMODEL_EDITOR
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    AZ::AzToolsFramework
                    Gem::GraphCanvasWidgets
                    Gem::GraphModel.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::GraphModel.Editor.Tests
        )
    endif()
endif()
