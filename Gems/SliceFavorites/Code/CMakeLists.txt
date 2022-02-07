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
    NAME SliceFavorites.Editor GEM_MODULE

    NAMESPACE Gem
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

# the above module is for use in Tools only (no need to load it in builders)
ly_create_alias(NAME SliceFavorites.Tools    NAMESPACE Gem TARGETS Gem::SliceFavorites.Editor)