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
    NAME EditorCommon SHARED
    NAMESPACE Legacy
    AUTOMOC
    FILES_CMAKE
        editorcommon_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    COMPILE_DEFINITIONS
        PRIVATE
            EDITOR_COMMON_EXPORTS
        INTERFACE
            EDITOR_COMMON_IMPORTS
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Widgets
            Legacy::CryCommon
            Legacy::EditorCore
            AZ::AzCore
            AZ::AzToolsFramework
            AZ::AzQtComponents
    RUNTIME_DEPENDENCIES
        AZ::AzCore
        AZ::AzToolsFramework
        AZ::AzQtComponents
        Legacy::EditorCore
)
