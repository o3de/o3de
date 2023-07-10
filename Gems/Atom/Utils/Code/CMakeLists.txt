#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME Atom_Utils.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_utils_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::Atom_RHI.Public
            AzFramework
        PUBLIC
            Gem::Atom_RHI.Reflect
            Gem::Atom_RPI.Public
            Gem::ImGui.imguilib
            3rdParty::PNG
            Gem::ProfilerImGui
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Atom_Utils.TestUtils.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            atom_utils_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AtomCore
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

    ly_add_target(
        NAME Atom_Utils.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            atom_utils_tests_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                Gem::Atom_Utils.Static
    )
    ly_add_googletest(
        NAME Gem::Atom_Utils.Tests
        LABELS REQUIRES_tiaf
    )
endif()
