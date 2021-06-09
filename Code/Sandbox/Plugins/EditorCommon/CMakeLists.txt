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
            3rdParty::zlib
            3rdParty::Qt::Core
            3rdParty::Qt::Widgets
            Legacy::CryCommon
            Legacy::EditorCore
            AZ::AzCore
            AZ::AzToolsFramework
            AZ::AzQtComponents
)
