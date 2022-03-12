#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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

# the script canvas debugger is an optional gem module
# To Enable it, associate it with a project
ly_create_alias(NAME ScriptCanvasDebugger.Builders NAMESPACE Gem TARGETS Gem::ScriptCanvasDebugger)
ly_create_alias(NAME ScriptCanvasDebugger.Tools    NAMESPACE Gem TARGETS Gem::ScriptCanvasDebugger)
ly_create_alias(NAME ScriptCanvasDebugger.Clients  NAMESPACE Gem TARGETS Gem::ScriptCanvasDebugger)
ly_create_alias(NAME ScriptCanvasDebugger.Servers  NAMESPACE Gem TARGETS Gem::ScriptCanvasDebugger)

ly_add_target(
    NAME ScriptCanvas.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvasgem_headers.cmake
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
        *.ScriptCanvasGrammar.xml,ScriptCanvasGrammar_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasGrammar.xml,ScriptCanvasGrammar_Source.jinja,$path/$fileprefix.generated.cpp
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Source.jinja,$path/$fileprefix.generated.cpp
    RUNTIME_DEPENDENCIES
        Gem::ScriptCanvasDebugger
)

ly_add_target(
    NAME ScriptCanvas.API HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvasgem_headers.cmake
    COMPILE_DEFINITIONS
        INTERFACE
            SCRIPTCANVAS_ERRORS_ENABLED
            ${SCRIPT_CANVAS_COMMON_DEFINES}
    INCLUDE_DIRECTORIES
        INTERFACE
            .
            Include
            ${SCRIPT_CANVAS_AUTOGEN_BUILD_DIR}
)

ly_add_target(
    NAME ScriptCanvas ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
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
    RUNTIME_DEPENDENCIES
        Gem::ScriptEvents
        Gem::ExpressionEvaluation
)

# the "ScriptCanvas" target is active in Clients and Servers
ly_create_alias(NAME ScriptCanvas.Clients  NAMESPACE Gem TARGETS Gem::ScriptCanvas)
ly_create_alias(NAME ScriptCanvas.Servers  NAMESPACE Gem TARGETS Gem::ScriptCanvas)

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
                Editor/Assets
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
            scriptcanvasgem_editor_tools_files.cmake
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
                Tools
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
        RUNTIME_DEPENDENCIES
            Gem::ScriptCanvas
    )

    ly_add_target(
        NAME ScriptCanvas.Editor GEM_MODULE

        NAMESPACE Gem
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
            Gem::GraphCanvas.Editor
            Gem::ScriptEvents.Editor
            Gem::ExpressionEvaluation
    )

    # the "ScriptCanvas.Editor" target is active in all dev tools:
    ly_create_alias(NAME ScriptCanvas.Builders NAMESPACE Gem TARGETS Gem::ScriptCanvas.Editor)
    ly_create_alias(NAME ScriptCanvas.Tools    NAMESPACE Gem TARGETS Gem::ScriptCanvas.Editor)


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
        RUNTIME_DEPENDENCIES
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
