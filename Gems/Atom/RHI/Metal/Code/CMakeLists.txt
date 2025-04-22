#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_include_dir ${CMAKE_CURRENT_LIST_DIR}/Include/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_source_dir}/PAL2_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_ATOM_RHI_METAL_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Private ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            atom_rhi_metal_stub_module.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                ${pal_include_dir}
                Source
                ${pal_source_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Gem::Atom_RHI.Reflect
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
        Source/Platform/Common/Unimplemented/ModuleStub_Unimplemented.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ${gem_name}.Builders GEM_MODULE
            NAMESPACE Gem
            FILES_CMAKE
                Source/Platform/${PAL_PLATFORM_NAME}/platform_builders_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
                atom_rhi_metal_reflect_common_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Include
                    ${pal_include_dir}
                    Source
                    ${pal_source_dir}
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzCore
                    Gem::Atom_RHI.Reflect
                    Gem::Atom_RHI.Public
        )
    endif()

    return()
endif()

ly_add_target(
    NAME ${gem_name}.Reflect STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_metal_reflect_common_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            ${pal_include_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::Atom_RHI.Reflect
)

ly_add_target(
    NAME ${gem_name}.Private.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_metal_private_common_files.cmake
        Source/Platform/${PAL_PLATFORM_NAME}/platform_private_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        Source/Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            ${pal_source_dir}
        PUBLIC
            Include
            ${pal_include_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_RHI.Reflect
            Gem::${gem_name}.Reflect
)

ly_add_target(
    NAME ${gem_name}.Private ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_metal_private_common_shared_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${pal_source_dir}/platform_shared_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            ${pal_source_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::Atom_RHI.Reflect
            Gem::Atom_RHI.Public
            Gem::${gem_name}.Reflect
            Gem::${gem_name}.Private.Static
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/RHI/Module.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

if (PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME ${gem_name}.Builders.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            atom_rhi_metal_builders_common_files.cmake
            ${pal_include_dir}/platform_builders_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
            ${pal_source_dir}/platform_builders_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                ${pal_source_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AssetBuilderSDK
                Gem::Atom_RHI.Edit
                Gem::Atom_RHI.Reflect
                Gem::${gem_name}.Reflect
    )

   ly_add_target(
        NAME ${gem_name}.Builders GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            atom_rhi_metal_builders_shared_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            ${pal_source_dir}/platform_shared_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AssetBuilderSDK
                Gem::Atom_RHI.Edit
                Gem::Atom_RHI.Reflect
                Gem::${gem_name}.Reflect
                Gem::${gem_name}.Builders.Static
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/RHI.Builders/BuilderModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

        ly_add_target(
            NAME ${gem_name}.Builders.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                atom_rhi_metal_builders_tests_files.cmake
            PLATFORM_INCLUDE_FILES
                ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
                ${pal_source_dir}/platform_shared_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Include
                    Source
                    ${pal_source_dir}
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::Atom_RHI.Reflect
                    Gem::${gem_name}.Reflect
                    Gem::${gem_name}.Builders.Static
        )

        ly_add_googletest(
            NAME Gem::${gem_name}.Builders.Tests
        )
    endif()
endif()

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            atom_rhi_metal_private_tests_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            ${pal_source_dir}/platform_shared_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
                ${pal_source_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::Atom_RHI.Reflect
                Gem::${gem_name}.Reflect
                Gem::${gem_name}.Private.Static
    )

    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
    )
endif()
