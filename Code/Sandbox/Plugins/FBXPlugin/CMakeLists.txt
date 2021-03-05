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

include(Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
if(NOT PAL_TRAIT_BUILD_FBX_PLUGIN_SUPPORTED)
    return()
endif()

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME FBXPlugin MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY EditorPlugins
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        fbxplugin_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            PLUGIN_EXPORTS
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
            3rdParty::FbxSdk
            Legacy::CryCommon
            Legacy::Editor.Headers
            Legacy::EditorCore
)

ly_add_dependencies(Editor FBXPlugin)
set_property(GLOBAL APPEND PROPERTY LY_EDITOR_PLUGINS $<TARGET_FILE_NAME:Legacy::FBXPlugin>)
