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

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME GridHub APPLICATION
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        gridhub_files.cmake
        Platform/${PAL_PLATFORM_NAME}/gridhub_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::OpenGL
            3rdParty::Qt::Network
            3rdParty::Qt::Svg
            3rdParty::Qt::Widgets
            AZ::AzCore
            AZ::GridMate
    COMPILE_DEFINITIONS 
        PRIVATE
            GRIDMATE_FOR_TOOLS
)
