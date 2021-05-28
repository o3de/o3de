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

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME ScriptCanvasTesting.Editor.Static STATIC
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
            Gem::ScriptCanvasEditor
            Gem::GraphCanvasWidgets
            Gem::ScriptEvents.Editor
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
            AZ::AssetBuilderSDK
        AUTOGEN_RULES
            *.ScriptCanvasGrammar.xml,ScriptCanvasGrammar_Header.jinja,$path/$fileprefix.generated.h
            *.ScriptCanvasGrammar.xml,ScriptCanvasGrammar_Source.jinja,$path/$fileprefix.generated.cpp
            *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Header.jinja,$path/$fileprefix.generated.h
            *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Source.jinja,$path/$fileprefix.generated.cpp
    RUNTIME_DEPENDENCIES
        Gem::ScriptCanvas.Editor
        Gem::ScriptCanvasEditor
        Gem::GraphCanvasWidgets
        Gem::ScriptEvents
)

ly_add_target(
    NAME ScriptCanvasTesting.Editor GEM_MODULE

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
            Gem::ScriptCanvasTesting.Editor.Static
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
    RUNTIME_DEPENDENCIES
        Gem::ScriptCanvas.Editor
)

# By default, the above module is used only in tools:
ly_create_alias(NAME ScriptCanvasTesting.Tools  NAMESPACE Gem TARGETS Gem::ScriptCanvasTesting.Editor)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ScriptCanvasTesting.Editor.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            scriptcanvastestingeditor_tests_files.cmake
        PLATFORM_INCLUDE_FILES
            Platform/Common/${PAL_TRAIT_COMPILER_ID}/scriptcanvastesting_editor_tests_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
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
                Gem::ScriptCanvasTesting.Editor.Static
                Gem::ScriptCanvas.Editor
        RUNTIME_DEPENDENCIES
            Gem::GraphCanvas.Editor
            Gem::ScriptCanvas.Editor
    )
    ly_add_googletest(
        NAME Gem::ScriptCanvasTesting.Editor.Tests
        TEST_SUITE smoke
    )
endif()


