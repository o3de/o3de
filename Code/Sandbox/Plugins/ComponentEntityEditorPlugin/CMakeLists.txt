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
    NAME ComponentEntityEditorPlugin MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY EditorPlugins
    AUTOMOC
    AUTOUIC
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

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME ComponentEntityEditorPlugin.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        AUTOMOC
        FILES_CMAKE
            componententityeditorplugin_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Core
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
                AZ::AzTest
                AZ::AzToolsFramework
                AZ::AzToolsFrameworkTestCommon
                Legacy::CryCommon
                Legacy::EditorLib
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )
    ly_add_googletest(
        NAME Legacy::ComponentEntityEditorPlugin.Tests
    )
endif()
