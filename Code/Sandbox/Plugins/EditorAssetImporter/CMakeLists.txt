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
    NAME EditorAssetImporter MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY EditorPlugins
    AUTOMOC
    AUTOUIC
    FILES_CMAKE
        editorassetimporter_files.cmake
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
            AZ::SceneCore
            AZ::SceneUI
            AZ::SceneData
            AZ::GFxFramework
            Legacy::CryCommon
            Legacy::EditorLib
            Legacy::EditorCommon
)

ly_add_dependencies(Editor EditorAssetImporter)
set_property(GLOBAL APPEND PROPERTY LY_EDITOR_PLUGINS $<TARGET_FILE_NAME:Legacy::EditorAssetImporter>)
