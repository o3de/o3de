#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

include(${pal_dir}/AtomRHITests_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(PAL_TRAIT_BUILD_RENDERDOC_SUPPORTED)
    set(LY_RENDERDOC_ENABLED OFF CACHE BOOL "Enable RenderDoc integration. Use LY_RENDERDOC_PATH to specific the path to RenderDoc.")
    if(LY_RENDERDOC_ENABLED)
        set(RENDERDOC_DEPENDENCY 3rdParty::renderdoc)
    endif()
endif()

ly_add_target(
    NAME Atom_RHI.Reflect STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_reflect_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
        PUBLIC
            AZ::AtomCore
)

ly_add_target(
    NAME Atom_RHI.Public STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_public_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        ${pal_source_dir}/platform_private_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_RHI.Reflect
        PUBLIC
            ${RENDERDOC_DEPENDENCY}
)

ly_add_target(
    NAME Atom_RHI.Private.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_private_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AtomCore
            Gem::Atom_RHI.Public
)

ly_add_target(
    NAME Atom_RHI.Private ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_private_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::Atom_RHI.Public
            Gem::Atom_RHI.Private.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    if (ATOM_RHI_TRAIT_BUILD_SUPPORTS_EDIT)
        ly_add_target(
            NAME Atom_RHI.Edit STATIC
            NAMESPACE Gem
            FILES_CMAKE
                atom_rhi_edit_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                PUBLIC
                    Include
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzToolsFramework
                    AZ::AtomCore
                    Gem::Atom_RHI.Public
                    3rdParty::md5
        )
    endif()
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    if (ATOM_RHI_TRAIT_BUILD_SUPPORTS_TEST)
        ly_add_target(
            NAME Atom_RHI.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                atom_rhi_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    Tests
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::Atom_RHI.Edit
                    Gem::Atom_RHI.Public
                    Gem::Atom_Feature_Common.Public
        )
        ly_add_googletest(
            NAME Gem::Atom_RHI.Tests
        )

        ly_add_target_files(
            TARGETS
                Atom_RHI.Tests
            FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/UtilsTestsData/DummyTransformColor.MainPS.dx12.dxil.txt
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/UtilsTestsData/DummyTransformColor.MainPS.vulkan.spirv.txt
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/UtilsTestsData/HelloWorld.txt
            OUTPUT_SUBDIRECTORY
                Gems/Atom/RHI/Code/Tests/UtilsTestsData
        )
    endif()
endif()
