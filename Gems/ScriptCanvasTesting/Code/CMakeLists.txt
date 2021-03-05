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
        PUBLIC
            Gem::ScriptCanvas
            Gem::ScriptCanvasEditor
            Gem::GraphCanvasWidgets
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
            AZ::AssetBuilderSDK
)

ly_add_target(
    NAME ScriptCanvasTesting.Editor MODULE
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
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ScriptCanvasTesting.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                Gem::ScriptCanvasTesting.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::GraphCanvas.Editor
            Gem::ScriptCanvasGem.Editor
    )
    ly_add_googletest(
        NAME Gem::ScriptCanvasTesting.Editor.Tests
    )
endif()


