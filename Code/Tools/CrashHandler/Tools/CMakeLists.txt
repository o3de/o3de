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

ly_add_target(
    NAME ToolsCrashHandler STATIC
    NAMESPACE AZ
    AUTOMOC
    FILES_CMAKE
        tools_crash_handler_files.cmake
        Platform/${PAL_PLATFORM_NAME}/tools_crash_handler_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            AZ::CrashHandler
            AZ::CrashSupport
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
            .
            Uploader
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
            3rdParty::Crashpad
            3rdParty::Crashpad::Handler
            AZ::CrashUploaderSupport
            AZ::AzQtComponents
            AZ::CrashSupport
    TARGET_PROPERTIES
        Qt5_NO_LINK_QTMAIN TRUE
)
