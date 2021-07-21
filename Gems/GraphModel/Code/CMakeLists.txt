#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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
        NAME GraphModel.Editor GEM_MODULE

        NAMESPACE Gem
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
        RUNTIME_DEPENDENCIES
            Gem::GraphCanvas.Editor
    )

    # Load the "Gem::GraphModel" module in Builders and Tools
    ly_create_alias(NAME GraphModel.Builders NAMESPACE Gem TARGETS Gem::GraphModel.Editor)
    ly_create_alias(NAME GraphModel.Tools    NAMESPACE Gem TARGETS Gem::GraphModel.Editor)

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
