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
    NAME ToolsCrashHandler STATIC
    NAMESPACE AZ
    AUTOMOC
    FILES_CMAKE
        tools_crash_handler_files.cmake
        Platform/${PAL_PLATFORM_NAME}/tools_crash_handler_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::CrashHandler
        PRIVATE
            3rdParty::Qt::Core
            AZ::AzToolsFramework
)

ly_add_target(
    NAME ToolsCrashUploader APPLICATION
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    FILES_CMAKE
        tools_crash_uploader_files.cmake
        Platform/${PAL_PLATFORM_NAME}/tools_crash_uploader_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Uploader
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
            AZ::CrashUploaderSupport
            AZ::AzQtComponents
    TARGET_PROPERTIES
        Qt5_NO_LINK_QTMAIN TRUE
)
