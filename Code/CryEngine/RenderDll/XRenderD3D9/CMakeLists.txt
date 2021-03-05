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

add_subdirectory(DXGL)
add_subdirectory(DXMETAL)

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

set(pal_tool_dirs "")
ly_get_pal_tool_dirs(pal_tool_roots ${CMAKE_CURRENT_LIST_DIR}/Platform)
foreach(include_path ${pal_tool_roots})
    list(APPEND pal_tool_dirs ${include_path}/../Common)
endforeach()


include(${pal_dir}/CryRender_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
ly_add_target(
    NAME CryRender.Headers HEADERONLY
    NAMESPACE Legacy
    FILES_CMAKE
        core_renderer_header_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/core_renderer_headers_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            ..
            .
            ${pal_dir}
    BUILD_DEPENDENCIES
        INTERFACE
            3rdParty::LZSS
            3rdParty::Lzma
)

################################################################################
# DX11
################################################################################
if(CRYRENDER_TRAIT_BUILD_DX11_SUPPORTED AND NOT LY_MONOLITHIC_GAME)  # only Atom is supported in monolithic
    ly_add_target(
        NAME CryRenderD3D11.Static STATIC
        NAMESPACE Legacy
        FILES_CMAKE
            core_renderer_files.cmake
            ${pal_dir}/core_renderer_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
            d3d11_files.cmake
            ${pal_dir}/d3d11_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/core_renderer_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            ${pal_dir}/d3d11_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                DIRECT3D10
                #XRENDERD3D10_EXPORTS
                DO_RENDERSTATS
                _RENDERER
                #EXCLUDE_CINEMIZER_SDK
        INCLUDE_DIRECTORIES
            PUBLIC
                .
                ../Common
                ..
                ${pal_dir}
            PRIVATE
                ${pal_tool_dirs}
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::LZSS
                3rdParty::Lzma
                Legacy::CryCommon
                Legacy::CryCommon.EngineSettings.Static
    )
    ly_add_source_properties(
        SOURCES
            ../Common/Shaders/PoundPoundParser.cpp
            ../Common/Renderer.cpp
            ../Common/Shaders/ParserBin.cpp
            ../Common/Shaders/RemoteCompiler.cpp
            ../Common/Shaders/ShaderCache.cpp
            ../Common/Textures/Image/DDSImage.cpp
        PROPERTY COMPILE_DEFINITIONS 
        VALUES ${LY_PAL_TOOLS_DEFINES}
    )

    ly_add_target(
        NAME CryRenderD3D11 ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            d3d11_shared_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/d3d11_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryRenderD3D11.Static
    )

    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

        ly_add_target(
            NAME CryRenderD3D11.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Legacy
            FILES_CMAKE
                d3d11_test_files.cmake
            PLATFORM_INCLUDE_FILES
                ${pal_dir}/d3d11_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Legacy::CryRenderD3D11.Static
        )
        ly_add_googletest(
            NAME Legacy::CryRenderD3D11.Tests
        )
    endif()
endif()

################################################################################
# DX12
################################################################################
if(CRYRENDER_TRAIT_BUILD_DX12_SUPPORTED AND NOT LY_MONOLITHIC_GAME) # only Atom is supported in monolithic

    ly_add_target(
        NAME CryRenderD3D12.Static STATIC
        NAMESPACE Legacy
        FILES_CMAKE
            core_renderer_files.cmake
            ${pal_dir}/core_renderer_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
            d3d12_files.cmake
            ${pal_dir}/d3d12_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/core_renderer_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            ${pal_dir}/d3d12_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                .
                ../Common
                ..
                DX12/Includes
                ${pal_dir}
            PRIVATE
                ${pal_tool_dirs}
        COMPILE_DEFINITIONS
            PUBLIC
                DIRECT3D10
                CRY_USE_DX12
                #XRENDERD3D10_EXPORTS
                DO_RENDERSTATS
                _RENDERER
                #EXCLUDE_CINEMIZER_SDK
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::LZSS
                3rdParty::Lzma
                Legacy::CryCommon
    )
    ly_add_source_properties(
        SOURCES
            ../Common/Shaders/RemoteCompiler.cpp
            ../Common/Textures/Image/DDSImage.cpp
        PROPERTY COMPILE_DEFINITIONS 
        VALUES ${LY_PAL_TOOLS_DEFINES}
    )

    ly_add_target(
        NAME CryRenderD3D12 ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            d3d12_shared_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/d3d12_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryRenderD3D12.Static
    )

    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

        ly_add_target(
            NAME CryRenderD3D12.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Legacy
            FILES_CMAKE
                d3d12_test_files.cmake
            PLATFORM_INCLUDE_FILES
                ${pal_dir}/d3d12_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Legacy::CryRenderD3D12.Static
        )
        # If the OS doesn't support DX12 runtime, a CTest test run will not be added as the shared library would fail to load
        if(CRYRENDER_TRAIT_RUNTIME_DX12_SUPPORTED)
                ly_add_googletest(
                    NAME Legacy::CryRenderD3D12.Tests
                )
        endif()
    endif()

endif()
