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

# Header only target to prevent linkage against editor libraries when it is not needed. Eventually the targets that depend
# on editor headers should cleanup dependencies and interact with the editor through buses or other mechanisms
ly_add_target(
    NAME EditorCommon.Headers HEADERONLY
    NAMESPACE Legacy
    FILES_CMAKE
        editorcommon_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            .
)

ly_add_target(
    NAME EditorCommon SHARED
    NAMESPACE Legacy
    AUTOMOC
    AUTORCC
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
