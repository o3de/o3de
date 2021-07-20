#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME AtomToolsFramework.Static STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        atomtoolsframework_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzToolsFramework
            AZ::AzQtComponents
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Network
            3rdParty::Qt::Widgets
            3rdParty::Python
            Gem::Atom_RPI.Edit
            Gem::Atom_RPI.Public
            Gem::Atom_RHI.Reflect
            Gem::Atom_Bootstrap.Headers
)

ly_add_target(
    NAME AtomToolsFramework.Editor GEM_MODULE

    NAMESPACE Gem
    AUTOMOC
    AUTORCC
    FILES_CMAKE
        atomtoolsframework_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AtomToolsFramework.Static
)
