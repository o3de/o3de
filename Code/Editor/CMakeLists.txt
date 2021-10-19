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

add_subdirectory(Plugins)

ly_add_target(
    NAME EditorCore SHARED
    NAMESPACE Legacy
    AUTOMOC
    AUTOUIC
    FILES_CMAKE
        editor_core_files.cmake
        Platform/${PAL_PLATFORM_NAME}/editor_core_files_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            .
            ..
    COMPILE_DEFINITIONS
        PRIVATE
            EDITOR_CORE
        PUBLIC
            EDITOR
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
        PUBLIC
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
            AZ::AzToolsFramework
)

# Header only target to prevent linkage against editor libraries when is not needed. Eventually the targets that depend
# on editor headers should cleanup dependencies and interact with the editor through buses or other mechanisms
ly_add_target(
    NAME Editor.Headers HEADERONLY
    NAMESPACE Legacy
    FILES_CMAKE
        editor_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
            .
            ..
    BUILD_DEPENDENCIES
        INTERFACE
            Legacy::EditorCommon.Headers
)

################################################################################
# EditorLib
################################################################################
set(pal_cmake_files "")
foreach(enabled_platform ${LY_PAL_TOOLS_ENABLED})
    string(TOLOWER ${enabled_platform} enabled_platform_lowercase)
    ly_get_list_relative_pal_filename(pal_cmake_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${enabled_platform})
    list(APPEND pal_cmake_files ${pal_cmake_dir}/editor_lib_${enabled_platform_lowercase}_files.cmake)
endforeach()

ly_get_pal_tool_dirs(pal_tool_dirs ${CMAKE_CURRENT_LIST_DIR}/Platform)

ly_add_target(
    NAME EditorLib SHARED
    NAMESPACE Legacy
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        editor_lib_files.cmake
        Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        Platform/${PAL_PLATFORM_NAME}/editor_lib_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        Platform/Common/${PAL_TRAIT_COMPILER_ID}/editor_lib_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        ${pal_cmake_files}
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            .
            ..
            AssetBrowser/AssetTypes/Character
            ${pal_tool_dirs}
    COMPILE_DEFINITIONS
        PRIVATE
            SANDBOX_EXPORTS
            ${LY_LEGACY_TERRAIN_EDITOR_DEFINES}
        INTERFACE
            SANDBOX_IMPORTS
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
            3rdParty::Qt::Concurrent
            3rdParty::TIFF
            3rdParty::squish-ccr
            3rdParty::AWSNativeSDK::STS
            Legacy::CryCommon
            Legacy::EditorCommon
            AZ::AzCore
            AZ::AzToolsFramework
            Gem::LmbrCentral.Static
            AZ::AWSNativeSDKInit
            AZ::AtomCore
            Gem::Atom_RPI.Edit
            Gem::Atom_RPI.Public
            Gem::Atom_Feature_Common.Static
            Gem::AtomToolsFramework.Static
            Gem::AtomViewportDisplayInfo
            ${additional_dependencies}
        PUBLIC
            3rdParty::AWSNativeSDK::Core
            3rdParty::Qt::Network
            Legacy::EditorCore
    RUNTIME_DEPENDENCIES
        Gem::AtomViewportDisplayInfo
        Legacy::EditorCommon
)
ly_add_source_properties(
    SOURCES CryEdit.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES 
        O3DE_COPYRIGHT_YEAR=${LY_VERSION_COPYRIGHT_YEAR}
        LY_BUILD=${LY_VERSION_BUILD_NUMBER}
        ${LY_PAL_TOOLS_DEFINES}
)
ly_add_source_properties(
    SOURCES
        Core/LevelEditorMenuHandler.cpp
        MainWindow.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

get_property(editor_plugins GLOBAL PROPERTY LY_EDITOR_PLUGINS)
string (REPLACE ";" "," editor_plugins "${editor_plugins}")
ly_add_source_properties(
    SOURCES PluginManager.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES LY_EDITOR_PLUGINS="${editor_plugins}"
)

################################################################################
# Editor
################################################################################
ly_add_target(
    NAME Editor APPLICATION
    NAMESPACE Legacy
    AUTORCC
    FILES_CMAKE
        editor_files.cmake
    PLATFORM_INCLUDE_FILES
        Platform/${PAL_PLATFORM_NAME}/editor_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    TARGET_PROPERTIES
        LY_INSTALL_GENERATE_RUN_TARGET TRUE
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            Legacy::CryCommon
    RUNTIME_DEPENDENCIES
        Legacy::CrySystem
        Legacy::EditorLib
        ProjectManager
)

ly_set_gem_variant_to_load(TARGETS Editor VARIANTS Tools)
set_property(SOURCE
    CryEdit.cpp
    APPEND PROPERTY
        COMPILE_DEFINITIONS LY_CMAKE_TARGET="Editor"
)
ly_add_translations(
    TARGETS Editor
    PREFIX Translations
    FILES
        Translations/editor_en-us.ts 
        Translations/assetbrowser_en-us.ts 
)
ly_add_dependencies(Editor AssetProcessor)

if(LY_DEFAULT_PROJECT_PATH)
    set_property(TARGET Editor APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "--project-path=\"${LY_DEFAULT_PROJECT_PATH}\"")
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME EditorCore.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        AUTOMOC
        FILES_CMAKE
            editor_core_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
                Legacy::EditorCore
                Legacy::CryCommon
                AZ::AzCore
    )
    ly_add_googletest(
        NAME Legacy::EditorCore.Tests
    )

    ly_add_target(
        NAME EditorLib.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        AUTOMOC
        FILES_CMAKE
            editor_lib_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                .
        COMPILE_DEFINITIONS
            PRIVATE
                ${LY_LEGACY_TERRAIN_EDITOR_DEFINES}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                3rdParty::Qt::Core
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
                3rdParty::Qt::Test
                Legacy::CryCommon
                AZ::AzToolsFramework
                AZ::AzToolsFramework.Tests
                AZ::AzFrameworkTestShared
                AZ::AzToolsFrameworkTestCommon
                Legacy::EditorLib
                Gem::AtomToolsFramework.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral
    )
    ly_add_googletest(
        NAME Legacy::EditorLib.Tests
    )

    ly_add_target(
        NAME EditorLib.Camera.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            Lib/Tests/Camera/editor_lib_camera_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzTest
                AZ::AzToolsFramework
                AZ::AzTestShared
                Legacy::EditorLib
                Gem::Camera.Editor
                Gem::AtomToolsFramework.Static
        RUNTIME_DEPENDENCIES
            Legacy::EditorLib
    )

    ly_add_source_properties(
        SOURCES Lib/Tests/Camera/test_EditorCamera.cpp
        PROPERTY COMPILE_DEFINITIONS 
        VALUES CAMERA_EDITOR_MODULE="$<TARGET_FILE_BASE_NAME:Camera.Editor>"
    )

    ly_add_googletest(
        NAME Legacy::EditorLib.Camera.Tests
    )
endif()
