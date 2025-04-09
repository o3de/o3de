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
    NAME ComponentEntityEditorPlugin MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY EditorPlugins
    AUTOMOC
    AUTORCC
    FILES_CMAKE
        componententityeditorplugin_files.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            PLUGIN_EXPORTS
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Widgets
            AZ::AzCore
            AZ::AzToolsFramework
            Legacy::CryCommon
            Legacy::EditorLib
            AZ::AtomCore
            Gem::Atom_RPI.Public
            Gem::AtomToolsFramework.Static
            Gem::LmbrCentral.Editor
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral.Editor
)

ly_add_dependencies(Editor ComponentEntityEditorPlugin)
set_property(GLOBAL APPEND PROPERTY LY_EDITOR_PLUGINS $<TARGET_FILE_NAME:Legacy::ComponentEntityEditorPlugin>)