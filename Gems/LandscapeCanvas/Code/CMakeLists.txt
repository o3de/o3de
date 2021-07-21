#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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
                Gem::GraphModel.Editor.Static
                Gem::GradientSignal.Editor
                Gem::SurfaceData.Editor
                Gem::Vegetation.Editor
                Gem::LmbrCentral.Editor
            PUBLIC
                Gem::GraphCanvasWidgets
        RUNTIME_DEPENDENCIES
            Gem::GradientSignal.Editor
            Gem::SurfaceData.Editor
            Gem::Vegetation.Editor
            Gem::LmbrCentral.Editor
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

    # by default, load the above "Gem::LandscapeCanvas.Editor" module in dev applications
    ly_create_alias(NAME LandscapeCanvas.Builders NAMESPACE Gem TARGETS Gem::LandscapeCanvas.Editor)
    ly_create_alias(NAME LandscapeCanvas.Tools    NAMESPACE Gem TARGETS Gem::LandscapeCanvas.Editor)

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
                    Gem::GraphModel.Editor.Static
                    Gem::LandscapeCanvas.Editor.Static
            RUNTIME_DEPENDENCIES
                Gem::GraphCanvasWidgets
        )
        ly_add_googletest(
            NAME Gem::LandscapeCanvas.Editor.Tests
        )
    endif()
endif()
