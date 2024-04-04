#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_dir}/AtomRHITests_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

ly_add_target(
    NAME Atom_RHI.Profiler STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_profiler_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
)

if(PAL_TRAIT_BUILD_RENDERDOC_SUPPORTED)  
    set(LY_RENDERDOC_ENABLED OFF CACHE BOOL "Enable RenderDoc integration. Use LY_RENDERDOC_PATH to specific the path to RenderDoc.")
    if(LY_RENDERDOC_ENABLED)
        message(STATUS "Renderdoc found, enabling as a graphics debugger")
        ly_add_target(
            NAME Atom_RHI.Profiler.RenderDoc ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                atom_rhi_profiler_renderdoc_shared_files.cmake
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
                    Gem::Atom_RHI.Profiler
                    3rdParty::renderdoc
        )

        list(APPEND PROFILER_DEPENDENCIES Atom_RHI.Profiler.RenderDoc)
    endif()
endif()

if(PAL_TRAIT_PROF_PIX_SUPPORTED)
    if(LY_PIX_ENABLED)
        ly_add_target(
            NAME Atom_RHI.Profiler.PIX ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                atom_rhi_profiler_pix_shared_files.cmake
                ${pal_source_dir}/platform_pix_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
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
                    Gem::Atom_RHI.Profiler
                    3rdParty::pix
        )

        list(APPEND PROFILER_DEPENDENCIES Atom_RHI.Profiler.PIX)
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
            Gem::Atom_RHI.Profiler
    RUNTIME_DEPENDENCIES
        ${PROFILER_DEPENDENCIES}            
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
            LABELS REQUIRES_tiaf
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
