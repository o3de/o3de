#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME ${gem_name}.Editor.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvastestingeditor_files.cmake
        scriptcanvastesting_autogen_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            .
    COMPILE_DEFINITIONS
        PRIVATE
            SCRIPTCANVAS_EDITOR
            SCRIPTCANVAS
            SCRIPTCANVAS_ERRORS_ENABLED
            ENABLE_EXTENDED_MATH_SUPPORT=0
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ScriptCanvas
            Gem::ScriptCanvas.Editor
            Gem::GraphCanvasWidgets
            Gem::ScriptEvents.Editor
            Gem::ScriptCanvas.Extensions
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
            AZ::AssetBuilderSDK
    AUTOGEN_RULES
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Source.jinja,$path/$fileprefix.generated.cpp
        *.ScriptCanvasFunction.xml,ScriptCanvasFunction_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasFunction.xml,ScriptCanvasFunction_Source.jinja,$path/$fileprefix.generated.cpp
    RUNTIME_DEPENDENCIES
        Gem::ScriptCanvas.Editor
        Gem::GraphCanvasWidgets
        Gem::ScriptEvents
)

ly_add_target(
    NAME ${gem_name}.Editor GEM_MODULE

    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvastestingeditor_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
    COMPILE_DEFINITIONS
        PRIVATE
            SCRIPTCANVAS_EDITOR
            SCRIPTCANVAS
            SCRIPTCANVAS_ERRORS_ENABLED
            ENABLE_EXTENDED_MATH_SUPPORT=0
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Editor.Static
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
    RUNTIME_DEPENDENCIES
        Gem::ScriptCanvas.Editor
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/ScriptCanvasTestingEditorModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# By default, the above module is used only in tools:
ly_create_alias(NAME ${gem_name}.Tools  NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
ly_create_alias(NAME ${gem_name}.Builders  NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Editor.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            scriptcanvastestingeditor_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                Tests
        COMPILE_DEFINITIONS
            PRIVATE
                SCRIPTCANVAS_EDITOR
                SCRIPTCANVAS
                SCRIPTCANVAS_ERRORS_ENABLED
                ENABLE_EXTENDED_MATH_SUPPORT=0
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                AZ::AzToolsFramework
                Gem::${gem_name}.Editor.Static
                Gem::ScriptCanvas.Editor
        RUNTIME_DEPENDENCIES
            Gem::GraphCanvas.Editor
            Gem::ScriptCanvas.Editor
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.Editor.Tests
    )
endif()


