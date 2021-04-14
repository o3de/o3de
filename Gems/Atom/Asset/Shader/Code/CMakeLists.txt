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

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) #for PAL_TRAIT_BUILD_ATOM_ASSET_SHADER_SUPPORTED

if(NOT PAL_TRAIT_BUILD_ATOM_ASSET_SHADER_SUPPORTED)

    # Create a stub
    ly_add_target(
        NAME Atom_Asset_Shader.Builders GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            atom_asset_shader_builders_stub_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
    )

    return()

endif()


ly_add_target(
    NAME Atom_Asset_Shader.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_asset_shader_builders_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${common_source_dir}/${PAL_TRAIT_COMPILER_ID}/atom_asset_shader_static_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            Source/Editor
            ${pal_source_dir}
    COMPILE_DEFINITIONS
        PRIVATE
            NOT_USE_CRY_MEMORY_MANAGER
            _SCL_SECURE_NO_WARNINGS
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::mcpp
            AZ::AssetBuilderSDK
            AZ::AtomCore
            AZ::AzCore
            AZ::AzFramework
            AZ::AzFramework
            AZ::AzToolsFramework
            Gem::Atom_RHI.Edit
            Gem::Atom_RPI.Public
)

set(builder_tools_include_files)
foreach(enabled_platform ${LY_PAL_TOOLS_ENABLED})
    string(TOLOWER ${enabled_platform} enabled_platform_lowercase)
    ly_get_list_relative_pal_filename(builder_tools_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${enabled_platform})
    list(APPEND builder_tools_include_files ${builder_tools_source_dir}/platform_builders_${enabled_platform_lowercase}.cmake)
endforeach()

ly_add_target(
    NAME Atom_Asset_Shader.Builders GEM_MODULE

    NAMESPACE Gem
    FILES_CMAKE
        atom_asset_shader_builders_shared_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_builders_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${builder_tools_include_files}
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            Source/Editor
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::mcpp
            Gem::Atom_Asset_Shader.Static
            Gem::Atom_RPI.Edit
    RUNTIME_DEPENDENCIES
        3rdParty::DirectXShaderCompilerDxc
        3rdParty::DirectXShaderCompilerDxcAz
        3rdParty::SPIRVCross
        3rdParty::azslc
)
