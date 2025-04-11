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
#   SCRIPT_CANVAS_DEBUG_DEBUGGER # to debug the ${gem_name} Debugging systems themselves
>
)

ly_add_target(
    NAME ${gem_name}Debugger STATIC
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
ly_create_alias(NAME ${gem_name}Debugger.Builders NAMESPACE Gem TARGETS Gem::${gem_name}Debugger)
ly_create_alias(NAME ${gem_name}Debugger.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}Debugger)
ly_create_alias(NAME ${gem_name}Debugger.Clients  NAMESPACE Gem TARGETS Gem::${gem_name}Debugger)
ly_create_alias(NAME ${gem_name}Debugger.Servers  NAMESPACE Gem TARGETS Gem::${gem_name}Debugger)
ly_create_alias(NAME ${gem_name}Debugger.Unified  NAMESPACE Gem TARGETS Gem::${gem_name}Debugger)

ly_add_target(
    NAME ${gem_name}.Extensions STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvasgem_extensions_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

ly_add_target(
    NAME ${gem_name}.API STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvasgem_autogen_files.cmake
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
            Gem::${gem_name}.Extensions
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::${gem_name}Debugger
    AUTOGEN_RULES
        *.ScriptCanvasGrammar.xml,ScriptCanvasGrammar_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasGrammar.xml,ScriptCanvasGrammar_Source.jinja,$path/$fileprefix.generated.cpp
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Source.jinja,$path/$fileprefix.generated.cpp
        *.ScriptCanvasFunction.xml,ScriptCanvasFunction_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasFunction.xml,ScriptCanvasFunction_Source.jinja,$path/$fileprefix.generated.cpp

    RUNTIME_DEPENDENCIES
        Gem::${gem_name}Debugger
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvasgem_game_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Include/${gem_name}
    COMPILE_DEFINITIONS
        PUBLIC
            SCRIPTCANVAS_ERRORS_ENABLED
            SCRIPTCANVAS_EDITOR
        PRIVATE
            ${SCRIPT_CANVAS_COMMON_DEFINES}
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::${gem_name}.API
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::ScriptEvents.Static
    RUNTIME_DEPENDENCIES
        Gem::ScriptEvents
        Gem::ExpressionEvaluation
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/ScriptCanvasGem.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# the "${gem_name}" target is active in Clients and Servers
ly_create_alias(NAME ${gem_name}.Clients  NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers  NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified  NAMESPACE Gem TARGETS Gem::${gem_name})

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        AUTORCC
        FILES_CMAKE
            scriptcanvasgem_editor_files.cmake
            scriptcanvasgem_editor_asset_files.cmake
            scriptcanvasgem_editor_builder_files.cmake
            scriptcanvasgem_editor_tools_files.cmake
            scriptcanvasgem_game_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                SCRIPTCANVAS_ERRORS_ENABLED
            PRIVATE
                ${SCRIPT_CANVAS_COMMON_DEFINES}
        INCLUDE_DIRECTORIES
            PUBLIC
                Editor/Include
            PRIVATE
                .
                Editor
                Tools
                ${SCRIPT_CANVAS_AUTOGEN_BUILD_DIR}
        BUILD_DEPENDENCIES  
            PUBLIC
                Legacy::CryCommon
                AZ::AzCore
                AZ::AssetBuilderSDK
                ${additional_dependencies}
                Gem::${gem_name}.API
                Gem::ScriptEvents.Static
                Gem::GraphCanvasWidgets
                Gem::ExpressionEvaluation.Static
            PRIVATE
                Legacy::EditorCore
        RUNTIME_DEPENDENCIES
            Gem::${gem_name}
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
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
            PUBLIC
                Editor/Include
            PRIVATE
                .
                Editor
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzToolsFramework
            PUBLIC
                Gem::${gem_name}.Editor.Static
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

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Editor/ScriptCanvasEditorGem.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    # the "${gem_name}.Editor" target is active in all dev tools:
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)


endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            scriptcanvasgem_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzCoreTestCommon
                AZ::AzFramework
                AZ::AzTest
                Gem::${gem_name}.API
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
    )

    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests MODULE
            NAMESPACE Gem
            FILES_CMAKE
                scriptcanvasgem_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Builder
                    Editor
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzCoreTestCommon
                    AZ::AzTest
                    Gem::${gem_name}.API
                    Gem::${gem_name}.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
        )
    endif()
endif()
