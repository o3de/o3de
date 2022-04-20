#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(SCRIPT_CANVAS_DEV_COMMON_DEFINES
    SCRIPTCANVASDEVELOPER
    AZCORE_ENABLE_MEMORY_TRACKING
)

ly_add_target(
    NAME ScriptCanvasDeveloper.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvasdeveloper_gem_common_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    COMPILE_DEFINITIONS
        PRIVATE
            ${SCRIPT_CANVAS_DEV_COMMON_DEFINES}
            IMGUI_ENABLED
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ScriptCanvas
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::ImGui.imguilib
            Gem::ImGui            
)

ly_add_target(
    NAME ScriptCanvasDeveloper ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvasdeveloper_gem_game_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    COMPILE_DEFINITIONS
        PRIVATE
            ${SCRIPT_CANVAS_DEV_COMMON_DEFINES}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::ScriptCanvasDeveloper.Static
    RUNTIME_DEPENDENCIES
        Gem::ScriptCanvas
)

# By default, the above module is the Client/Server module
ly_create_alias(NAME ScriptCanvasDeveloper.Clients NAMESPACE Gem TARGETS Gem::ScriptCanvasDeveloper)
ly_create_alias(NAME ScriptCanvasDeveloper.Servers NAMESPACE Gem TARGETS Gem::ScriptCanvasDeveloper)


if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ScriptCanvasDeveloper.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            scriptcanvasdeveloper_gem_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                Editor/Include
                Editor/Source
        COMPILE_DEFINITIONS
            PRIVATE
                SCRIPTCANVASDEVELOPER_EDITOR
                ${SCRIPT_CANVAS_DEV_COMMON_DEFINES}
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Test
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                Gem::ScriptCanvasDeveloper.Static
                Gem::ScriptCanvas.Editor.Static
                Gem::GraphCanvasWidgets
        RUNTIME_DEPENDENCIES
            Gem::ScriptCanvas.Editor
            Gem::GraphCanvasWidgets
    )
    # By Default the above module is the dev tools module
    ly_create_alias(NAME ScriptCanvasDeveloper.Builders NAMESPACE Gem TARGETS Gem::ScriptCanvasDeveloper.Editor)
    ly_create_alias(NAME ScriptCanvasDeveloper.Tools    NAMESPACE Gem TARGETS Gem::ScriptCanvasDeveloper.Editor)

endif()
