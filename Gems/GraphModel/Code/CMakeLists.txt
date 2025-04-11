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
        NAME ${gem_name}.Editor GEM_MODULE

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
                Gem::${gem_name}.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::GraphCanvas.Editor
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/GraphModelModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    # Load the "Gem::${gem_name}" module in Builders and Tools
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests MODULE
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
                    Gem::${gem_name}.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
            LABELS REQUIRES_tiaf
        )
    endif()
endif()
