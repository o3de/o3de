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
        NAME LandscapeCanvas.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            landscapecanvas_editor_static_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                LANDSCAPECANVAS_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Core
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
                AZ::AzCore
                AZ::AzToolsFramework
                Legacy::CryCommon
                Legacy::Editor.Headers
                Legacy::EditorCommon
                Gem::LmbrCentral
                Gem::GraphCanvasWidgets
                Gem::GraphModel.Editor.Static
                Gem::GradientSignal.Editor
                Gem::SurfaceData.Editor
                Gem::Vegetation.Editor
    )
    ly_add_target(
        NAME LandscapeCanvas.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            landscapecanvas_editor_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                LANDSCAPECANVAS_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzToolsFramework
                Legacy::Editor.Headers
                Gem::GraphCanvasWidgets
                Gem::GraphModel.Editor.Static
                Gem::LandscapeCanvas.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::GraphCanvas.Editor
            Gem::LmbrCentral.Editor
            Gem::GraphModel.Editor
            Gem::GradientSignal.Editor
            Gem::SurfaceData.Editor
            Gem::Vegetation.Editor
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME LandscapeCanvas.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                landscapecanvas_tests_editor_files.cmake
            COMPILE_DEFINITIONS
                PRIVATE
                    LANDSCAPECANVAS_EDITOR
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
                    Gem::LandscapeCanvas.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::LandscapeCanvas.Editor.Tests
        )
    endif()
endif()
