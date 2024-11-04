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
    NAME ${gem_name}.Static STATIC
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
            Gem::ScriptCanvas.API
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::ImGui.imguilib           
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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
            Gem::${gem_name}.Static
            Gem::ImGui  # note that this includes the ImGui bus interfaces, but not necessarily the static lib 
    RUNTIME_DEPENDENCIES
        Gem::ScriptCanvas
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Game/Source/ScriptCanvasDeveloperGem.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# By default, the above module is the Client/Server module
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})


if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
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
                Gem::${gem_name}.Static
                Gem::ScriptCanvas.Editor.Static
                Gem::GraphCanvasWidgets
                Gem::ImGui.Tools  # note that this includes the ImGui bus interfaces, but not necessarily the static lib 
        RUNTIME_DEPENDENCIES
            Gem::ScriptCanvas.Editor
            Gem::GraphCanvasWidgets
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Editor/Source/ScriptCanvasDeveloperGem.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})
    # By Default the above module is the dev tools module
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

endif()
