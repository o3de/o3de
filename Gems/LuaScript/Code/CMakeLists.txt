# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
# Description: build Terrain and Terrain.Static.
# Create: 2021-07-14

ly_add_target(
        NAME LuaScript.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            luascript_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Legacy::CryCommon
                3rdParty::Qt::Core
                3rdParty::Qt::Widgets
                3rdParty::Qt::Gui
                AZ::AzFramework
                AZ::AzToolsFramework
)

ly_add_target(
        NAME LuaScript ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            luascript_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommon
                Gem::LuaScript.Static
)

# clients and servers use the above module:
ly_create_alias(NAME LuaScript.Clients NAMESPACE Gem TARGETS Gem::LuaScript)
# ly_create_alias(NAME LuaScript.Servers NAMESPACE Gem TARGETS Gem::LuaScript)

if (PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME LuaScript.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            luascript_editor_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                HWNAV_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommon
                Legacy::Editor.Headers
                Legacy::EditorCommon
                AZ::AzToolsFramework
                Gem::LuaScript.Static
                Gem::LmbrCentral.Static
    )

    # tools and builders use the above module.
    ly_create_alias(NAME LuaScript.Tools NAMESPACE    Gem TARGETS Gem::LuaScript.Editor)
    ly_create_alias(NAME LuaScript.Builders NAMESPACE Gem TARGETS Gem::LuaScript.Editor)

    # The DefaultPrefab contains an EditorCameraComponent which makes this gem required
    ly_enable_gems(GEMS LuaScript VARIANTS Tools TARGETS Editor)
    # ly_enable_gems(GEMS LuaScript VARIANTS Builders TARGETS AssetBuilder AssetProcessor AssetProcessorBatch)

endif()
