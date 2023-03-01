#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


# DocumentEditor
ly_add_target(
    NAME DocumentEditor STATIC
    #NAMESPACE AzToolsFramework
    AUTOMOC
    #AUTOUIC
    AUTORCC
    FILES_CMAKE
        documenteditor_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    COMPILE_DEFINITIONS
        PRIVATE
            $<$<CONFIG:debug>:ENABLE_UNDOCACHE_CONSISTENCY_CHECKS>
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::SQLite
            AZ::AzCore
        PUBLIC
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
            AZ::AzFramework
            AZ::AzQtComponents
)