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
    NAME ${gem_name}.Profiler STATIC
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
            NAME ${gem_name}.Profiler.RenderDoc ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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
                    Gem::${gem_name}.Profiler
                    3rdParty::renderdoc
        )

        list(APPEND PROFILER_DEPENDENCIES ${gem_name}.Profiler.RenderDoc)
    endif()
endif()

if(PAL_TRAIT_PROF_PIX_SUPPORTED)
    if(LY_PIX_ENABLED)
        ly_add_target(
            NAME ${gem_name}.Profiler.PIX ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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
                    Gem::${gem_name}.Profiler
                    3rdParty::pix
        )

        list(APPEND PROFILER_DEPENDENCIES ${gem_name}.Profiler.PIX)
    endif()
endif()

ly_add_target(
    NAME ${gem_name}.Reflect STATIC
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
    NAME ${gem_name}.Public STATIC
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
            Gem::${gem_name}.Reflect
)

ly_add_target(
    NAME ${gem_name}.Private.Static STATIC
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
            Gem::${gem_name}.Public
)

ly_add_target(
    NAME ${gem_name}.Private ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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
            Gem::${gem_name}.Public
            Gem::${gem_name}.Private.Static
            Gem::${gem_name}.Profiler
    RUNTIME_DEPENDENCIES
        ${PROFILER_DEPENDENCIES}            
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/Module.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    if (ATOM_RHI_TRAIT_BUILD_SUPPORTS_EDIT)
        ly_add_target(
            NAME ${gem_name}.Edit STATIC
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
                    Gem::${gem_name}.Public
        )
    endif()
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    if (ATOM_RHI_TRAIT_BUILD_SUPPORTS_TEST)
        ly_add_target(
            NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                    Gem::${gem_name}.Edit
                    Gem::${gem_name}.Public
                    Gem::Atom_Feature_Common.Public
        )
        ly_add_googletest(
            NAME Gem::${gem_name}.Tests
            LABELS REQUIRES_tiaf
        )

        ly_add_target_files(
            TARGETS
                ${gem_name}.Tests
            FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/UtilsTestsData/DummyTransformColor.MainPS.dx12.dxil.txt
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/UtilsTestsData/DummyTransformColor.MainPS.vulkan.spirv.txt
                ${CMAKE_CURRENT_SOURCE_DIR}/Tests/UtilsTestsData/HelloWorld.txt
            OUTPUT_SUBDIRECTORY
                Gems/Atom/RHI/Code/Tests/UtilsTestsData
        )
    endif()
endif()
