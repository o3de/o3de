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

set(SCRIPT_CANVAS_COMMON_DEFINES 
    SCRIPTCANVAS
    ENABLE_EXTENDED_MATH_SUPPORT=0
)

set(SCRIPT_CANVAS_EXECUTION_NOTIFICATION_DEFINES
    $<$<NOT:$<CONFIG:Release>>:SC_EXECUTION_TRACE_ENABLED> # this is REQUIRED for debugging/execution logging
)

set(SCRIPT_CANVAS_DEBUG_DEBUGGER_DEFINES
$<$<NOT:$<CONFIG:Release>>:
#   SCRIPT_CANVAS_DEBUGGER_IS_ALWAYS_OBSERVING # for aggressive logging that ignores filtering (probably only for debug/development purposes)
#   SCRIPT_CANVAS_DEBUGGER_CONNECT_TO_ANYTHING_BUT_SELF # connect to the first target that is not self
#   SCRIPT_CANVAS_DEBUG_DEBUGGER # to debug the ScriptCanvas Debugging systems themselves
>
)

ly_add_target(
    NAME ScriptCanvasDebugger STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvasgem_debugger_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Include/ScriptCanvas
    COMPILE_DEFINITIONS
        PUBLIC
            SCRIPTCANVAS_ERRORS_ENABLED
        PRIVATE
            ${SCRIPT_CANVAS_COMMON_DEFINES}
            ${SCRIPT_CANVAS_DEBUG_DEBUGGER_DEFINES}
            ${SCRIPT_CANVAS_EXECUTION_NOTIFICATION_DEFINES}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::ScriptEvents.Static
)

ly_add_target(
    NAME ScriptCanvas.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvasgem_common_files.cmake
        scriptcanvasgem_runtime_asset_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            Include
            Include/ScriptCanvas
            ${SCRIPT_CANVAS_AUTOGEN_BUILD_DIR}
    COMPILE_DEFINITIONS
        PUBLIC
            SCRIPTCANVAS_ERRORS_ENABLED
        PRIVATE
            ${SCRIPT_CANVAS_COMMON_DEFINES}
            ${SCRIPT_CANVAS_DEBUG_DEBUGGER_DEFINES}
            ${SCRIPT_CANVAS_EXECUTION_NOTIFICATION_DEFINES}
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ScriptEvents.Static
            Gem::ExpressionEvaluation.Static
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::ScriptCanvasDebugger
    AUTOGEN_RULES
        *.ScriptCanvasNode.xml,ScriptCanvasNode_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasNode.xml,ScriptCanvasNode_Source.jinja,$path/$fileprefix.generated.cpp
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Source.jinja,$path/$fileprefix.generated.cpp
)

ly_add_target(
    NAME ScriptCanvas ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.ScriptCanvasGem.869a0d0ec11a45c299917d45c81555e6.v0.1.0
    FILES_CMAKE
        scriptcanvasgem_game_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Include/ScriptCanvas
    COMPILE_DEFINITIONS
        PUBLIC
            SCRIPTCANVAS_ERRORS_ENABLED
        PRIVATE
            ${SCRIPT_CANVAS_COMMON_DEFINES}
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ScriptCanvas.Static
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::ScriptEvents.Static
        
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ScriptCanvasEditor STATIC
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            scriptcanvasgem_editor_static_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                SCRIPTCANVAS_ERRORS_ENABLED
            PRIVATE
                SCRIPTCANVAS_EDITOR
                ${SCRIPT_CANVAS_COMMON_DEFINES}
        INCLUDE_DIRECTORIES
            PUBLIC
                .
                Editor/Include
                Editor/Static/Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzToolsFramework
                3rdParty::Qt::Widgets
                Gem::ScriptCanvas
    )

    ly_add_target(
        NAME ScriptCanvas.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        AUTORCC
        FILES_CMAKE
            scriptcanvasgem_editor_files.cmake
            scriptcanvasgem_editor_asset_files.cmake
            scriptcanvasgem_editor_builder_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                SCRIPTCANVAS_ERRORS_ENABLED
            PRIVATE
                SCRIPTCANVAS_EDITOR
                ${SCRIPT_CANVAS_COMMON_DEFINES}
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Editor
                Editor/Include
                ${SCRIPT_CANVAS_AUTOGEN_BUILD_DIR}
        BUILD_DEPENDENCIES  
            PUBLIC
                Legacy::CryCommon
                AZ::AzCore
                AZ::AssetBuilderSDK
                ${additional_dependencies}
                Gem::ScriptCanvas
                Gem::ScriptCanvasEditor
                Gem::ScriptEvents.Static
                Gem::GraphCanvasWidgets
                Gem::ExpressionEvaluation.Static
            PRIVATE
                Legacy::EditorCore
    )

    ly_add_target(
        NAME ScriptCanvas.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.ScriptCanvasGem.Editor.869a0d0ec11a45c299917d45c81555e6.v0.1.0
        FILES_CMAKE
            scriptcanvasgem_editor_shared_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                SCRIPTCANVAS_ERRORS_ENABLED
            PRIVATE
                SCRIPTCANVAS_EDITOR
                ${SCRIPT_CANVAS_COMMON_DEFINES}
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Editor
                Editor/Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzToolsFramework
            PUBLIC
                Gem::ScriptCanvas.Editor.Static
        RUNTIME_DEPENDENCIES
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
            Legacy::EditorCore
            AZ::AzQtComponents
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ScriptCanvas.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            scriptcanvasgem_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        COMPILE_DEFINITIONS
            PRIVATE
                SCRIPTCANVAS_EDITOR
                SCRIPTCANVAS_ERRORS_ENABLED
                ${SCRIPT_CANVAS_COMMON_DEFINES}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                Gem::ScriptCanvas
    )
    ly_add_googletest(
        NAME Gem::ScriptCanvas.Tests
    )

    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ScriptCanvas.Editor.Tests MODULE
            NAMESPACE Gem
            FILES_CMAKE
                scriptcanvasgem_tests_files.cmake
                scriptcanvasgem_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    Include
                    Include/ScriptCanvas
                    Source
            COMPILE_DEFINITIONS
                PRIVATE
                    SCRIPTCANVAS_EDITOR
                    SCRIPTCANVAS_ERRORS_ENABLED
                    ${SCRIPT_CANVAS_COMMON_DEFINES}
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::ScriptCanvasEditor
                    Gem::ScriptCanvas.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::ScriptCanvas.Editor.Tests
        )
    endif()
endif()
