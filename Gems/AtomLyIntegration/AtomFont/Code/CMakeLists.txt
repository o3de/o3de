#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

ly_add_target(
    NAME AtomFont ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atomfont_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Freetype
            AZ::AzCore
            AZ::AtomCore
            Legacy::CryCommon
            Gem::Atom_RHI.Reflect
            Gem::Atom_RPI.Public
        PUBLIC
            Gem::Atom_AtomBridge.Static
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME AtomFont.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            atomfont_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
    )
    ly_add_googletest(
        NAME Gem::AtomFont.Tests
    )
endif()

