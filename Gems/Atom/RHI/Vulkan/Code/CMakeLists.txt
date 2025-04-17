#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_include_dir ${CMAKE_CURRENT_LIST_DIR}/Include/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # PAL_TRAIT_ATOM_RHI_VULKAN_SUPPORTED / PAL_TRAIT_ATOM_RHI_VULKAN_TARGETS_ALREADY_DEFINED

if(PAL_TRAIT_AFTERMATH_AVAILABLE)
    set(USE_NSIGHT_AFTERMATH_DEFINE $<IF:$<CONFIG:release>,"","USE_NSIGHT_AFTERMATH">)
    set(AFTERMATH_BUILD_DEPENDENCY "3rdParty::Aftermath")
else()
    set(USE_NSIGHT_AFTERMATH_DEFINE "")
    set(AFTERMATH_BUILD_DEPENDENCY "")
endif()

if(PAL_TRAIT_ATOM_RHI_VULKAN_TARGETS_ALREADY_DEFINED)
    return() # Vulkan targets already defined in PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
endif()

set(ANDROID_USE_OCULUS_OPENXR OFF CACHE BOOL "Allows for custom behavior in the RHI related with issues that only occur on Quest devices.")
if(ANDROID_USE_OCULUS_OPENXR)
    message(STATUS "Some quirks for Quest devices may apply in the Vulkan RHI.")
    # It was found that as of April 2024 using Timeline Semaphores on Quest devices crashes the app.
    set(DISABLE_TIMELINE_SEMAPHORES_DEFINE "DISABLE_TIMELINE_SEMAPHORES") # Disables Timeline Semaphores even if the device claims to support them. 
endif()

ly_add_target(
    NAME ${gem_name}.Glad.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_vulkan_glad_files.cmake
        ${pal_include_dir}/platform_glad_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        ${pal_source_dir}/platform_glad_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            Include/Atom/RHI.Loader/Glad
            ${pal_include_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
        PUBLIC
            3rdParty::glad_vulkan
)

ly_add_target(
    NAME ${gem_name}.Reflect STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_vulkan_reflect_common_files.cmake
        ${pal_source_dir}/platform_reflect_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
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
            Gem::${gem_name}.Glad.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Builders.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            atom_rhi_vulkan_builders_common_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AssetBuilderSDK
                Gem::Atom_RHI.Reflect
                Gem::${gem_name}.Reflect
    )

    ly_add_target(
        NAME ${gem_name}.Builders GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            ${pal_source_dir}/platform_builders_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Gem::Atom_RHI.Reflect
                Gem::Atom_RHI.Public
                Gem::${gem_name}.Reflect
                Gem::${gem_name}.Builders.Static
                Gem::Atom_RHI.Edit
        )

endif()

if(NOT PAL_TRAIT_ATOM_RHI_VULKAN_SUPPORTED)

    # Create alias for Atom RHI Vulkan private Gem Module
    ly_create_alias(NAME ${gem_name}.Private NAMESPACE Gem)

    return() # Do not create the rest of the targets

endif()



ly_add_target(
    NAME ${gem_name}.Private.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_vulkan_private_common_files.cmake
        ${pal_source_dir}/platform_private_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        ${pal_include_dir}/platform_private_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_private_static_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_source_dir}
        PUBLIC
            Include
            ${pal_include_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            AZ::AzFramework.NativeUI
            Gem::Atom_RHI.Reflect
            Gem::${gem_name}.Reflect
            Gem::${gem_name}.Glad.Static
            3rdParty::amd_vma
            ${AFTERMATH_BUILD_DEPENDENCY}
    COMPILE_DEFINITIONS 
        PRIVATE
            ${USE_NSIGHT_AFTERMATH_DEFINE}
            ${DISABLE_TIMELINE_SEMAPHORES_DEFINE}
)

ly_add_target(
    NAME ${gem_name}.Interface STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_vulkan_interface_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_source_dir}
        PUBLIC
            Include
            ${pal_include_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::${gem_name}.Private.Static
)

ly_add_target(
    NAME ${gem_name}.Private ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_vulkan_private_common_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Private.Static
            Gem::Atom_RHI.Public
            ${VULKAN_VALIDATION_LAYER}
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/RHI/Module.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})
