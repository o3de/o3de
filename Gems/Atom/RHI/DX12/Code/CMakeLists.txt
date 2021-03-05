#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

ly_get_list_relative_pal_filename(pal_include_dir ${CMAKE_CURRENT_LIST_DIR}/Include/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # PAL_TRAIT_ATOM_RHI_DX12_SUPPORTED

if(PAL_TRAIT_PIX_AVAILABLE)
    set(USE_PIX_DEFINE "USE_PIX")
    set(PIX_BUILD_DEPENDENCY "3rdParty::pix")
else()
    set(USE_PIX_DEFINE "")
    set(PIX_BUILD_DEPENDENCY "")
endif()

set(pal_builder_tools_files)
set(pal_builder_tools_includes)
foreach(enabled_platform ${LY_PAL_TOOLS_ENABLED})
    string(TOLOWER ${enabled_platform} enabled_platform_lowercase)
    ly_get_list_relative_pal_filename(pal_builder_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${enabled_platform})
    list(APPEND pal_builder_tools_includes ${pal_builder_source_dir})
    list(APPEND pal_builder_tools_files ${pal_builder_source_dir}/platform_builders_${enabled_platform_lowercase}_tools_files.cmake)
endforeach()

if(NOT PAL_TRAIT_ATOM_RHI_DX12_SUPPORTED)
    
    # Create stub modules. Once we support gem loading configuration, we can remove this stubbed targets
    ly_add_target(
        NAME Atom_RHI_DX12.Private ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        OUTPUT_NAME Gem.Atom_RHI_DX12.Private.e011969cf32442fdaac2443a960ab5ff.v0.1.0
        FILES_CMAKE
            atom_rhi_dx12_stub_module.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
    )

    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME Atom_RHI_DX12.Builders MODULE
            NAMESPACE Gem
            OUTPUT_NAME Gem.Atom_RHI_DX12.Builders.e011969cf32442fdaac2443a960ab5ff.v0.1.0
            FILES_CMAKE
                Source/Platform/${PAL_PLATFORM_NAME}/platform_builders_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
                atom_rhi_dx12_reflect_common_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Include
                    Include/Platform/${PAL_PLATFORM_NAME}
                    Source
                    Source/Platform/${PAL_PLATFORM_NAME}
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzCore
                    AZ::AssetBuilderSDK
                    Gem::Atom_RHI.Edit
                    Gem::Atom_RHI.Reflect
                    3rdParty::d3dx12
        )
    endif()

    return() # Do not create the rest of the targets

endif()

ly_add_target(
    NAME Atom_RHI_DX12.Reflect STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_dx12_reflect_common_files.cmake
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
            3rdParty::d3dx12
            ${PIX_BUILD_DEPENDENCY}
            Gem::Atom_RHI.Reflect
    COMPILE_DEFINITIONS 
        PRIVATE
            ${USE_PIX_DEFINE}
)

ly_add_target(
    NAME Atom_RHI_DX12.Private.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rhi_dx12_private_common_files.cmake
        ${pal_source_dir}/platform_private_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_private_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Gem::Atom_RHI.Reflect
            Gem::Atom_RHI_DX12.Reflect
            3rdParty::d3dx12
            ${PIX_BUILD_DEPENDENCY}
    COMPILE_DEFINITIONS 
        PRIVATE
            ${USE_PIX_DEFINE}
)

ly_add_target(
    NAME Atom_RHI_DX12.Private ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Atom_RHI_DX12.Private.e011969cf32442fdaac2443a960ab5ff.v0.1.0
    FILES_CMAKE
        atom_rhi_dx12_private_common_shared_files.cmake
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
            Gem::Atom_RHI_DX12.Reflect
            Gem::Atom_RHI_DX12.Private.Static
            3rdParty::d3dx12
            ${PIX_BUILD_DEPENDENCY}
    COMPILE_DEFINITIONS 
        PRIVATE
            ${USE_PIX_DEFINE}
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Atom_RHI_DX12.Builders.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            ${pal_source_dir}/platform_builders_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
            ${pal_builder_tools_files}
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_source_dir}
                ${pal_builder_tools_includes}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AssetBuilderSDK
                Gem::Atom_RHI.Reflect
                Gem::Atom_RHI_DX12.Reflect
    )

    ly_add_target(
        NAME Atom_RHI_DX12.Builders MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Atom_RHI_DX12.Builders.e011969cf32442fdaac2443a960ab5ff.v0.1.0
        FILES_CMAKE
            atom_rhi_dx12_builders_common_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Gem::Atom_RHI.Edit
                Gem::Atom_RHI.Reflect
                Gem::Atom_RHI.Public
                Gem::Atom_RHI_DX12.Reflect
                Gem::Atom_RHI_DX12.Builders.Static
    )
endif()
