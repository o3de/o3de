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

include(${CMAKE_CURRENT_SOURCE_DIR}/Platform/${PAL_PLATFORM_NAME}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # For PAL_TRAIT_BUILD_P4PLUGIN_SUPPORTED

if(NOT PAL_TRAIT_BUILD_P4PLUGIN_SUPPORTED)
    return()
endif()

ly_add_target(
    NAME PerforcePlugin MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY EditorPlugins
    AUTOMOC
    AUTOUIC
    FILES_CMAKE
        perforceplugin_files.cmake
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
            Legacy::CryCommon
            Legacy::EditorLib
)

ly_add_dependencies(Editor PerforcePlugin)
set_property(GLOBAL APPEND PROPERTY LY_EDITOR_PLUGINS $<TARGET_FILE_NAME:Legacy::PerforcePlugin>)
