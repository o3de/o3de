#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_subdirectory(NewsBuilder)

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME NewsShared STATIC
    NAMESPACE Legacy
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        news_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Network
            3rdParty::Qt::Widgets
            AZ::AzQtComponents
)
