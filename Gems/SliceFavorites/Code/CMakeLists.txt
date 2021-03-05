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

ly_add_target(
    NAME SliceFavorites.Editor.Static STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        slicefavorites_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
            3rdParty::Qt::Core
            3rdParty::Qt::Widgets
            AZ::AzToolsFramework
)

ly_add_target(
    NAME SliceFavorites.Editor MODULE
    NAMESPACE Gem
    OUTPUT_NAME Gem.SliceFavorites.Editor.1bfc7270d4a1490daac8aa8b072c4489.v0.1.0
    FILES_CMAKE
        slicefavorites_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            Gem::SliceFavorites.Editor.Static
)
