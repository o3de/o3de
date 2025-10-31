#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor.Static STATIC
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
                Gem::LmbrCentral.Editor
            PUBLIC
                Gem::GraphCanvasWidgets
        RUNTIME_DEPENDENCIES
            Gem::GradientSignal.Editor
            Gem::SurfaceData.Editor
            Gem::LmbrCentral.Editor
    )
    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
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
                Gem::${gem_name}.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::GraphCanvas.Editor
            Gem::LmbrCentral.Editor
            Gem::GraphModel.Editor
            Gem::GradientSignal.Editor
            Gem::SurfaceData.Editor
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/LandscapeCanvasEditorModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    # by default, load the above "Gem::${gem_name}.Editor" module in dev applications
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                    Gem::${gem_name}.Editor.Static
            RUNTIME_DEPENDENCIES
                Gem::GraphCanvasWidgets
        )
        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
            LABELS REQUIRES_tiaf
        )
    endif()
endif()
