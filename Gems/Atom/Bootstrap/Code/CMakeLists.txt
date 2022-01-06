#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME Atom_Bootstrap.Headers HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        bootstrap_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
)

ly_add_target(
    NAME Atom_Bootstrap ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        bootstrap_files.cmake
        ${pal_dir}/bootstrap_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AtomCore
            AZ::AzCore
            AZ::AzFramework
            Legacy::CryCommon
            Gem::Atom_RPI.Public
)

# Atom_Bootstrap is only used in Launchers
ly_create_alias(NAME Atom_Bootstrap.Clients NAMESPACE Gem TARGETS Gem::Atom_Bootstrap)
ly_create_alias(NAME Atom_Bootstrap.Servers NAMESPACE Gem TARGETS Gem::Atom_Bootstrap)

# The Atom_Bootstrap gem is responsible for making the NativeWindow handle in the launcher applications
ly_enable_gems(GEMS Atom_Bootstrap)
