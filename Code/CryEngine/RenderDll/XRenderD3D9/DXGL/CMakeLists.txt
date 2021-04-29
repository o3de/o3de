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

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

include(${pal_dir}/DXGL_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(DXGL_TRAIT_BUILD_OPENGL_SUPPORTED AND LY_TRAIT_USE_LEGACY_IN_MONOLITHIC)
    ly_add_target(
        NAME CryRenderGL.Static STATIC
        NAMESPACE Legacy
        FILES_CMAKE
            ../core_renderer_files.cmake
            opengl_renderer_files.cmake
            ${pal_dir}/DXGL_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/DXGL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                DO_RENDERLOG
                DO_RENDERSTATS
                DIRECT3D10
                XRENDERGL_EXPORTS
                _RENDERER
                EXCLUDE_CINEMIZER_SDK
                OPENGL
                #XRENDERD3D10_EXPORTS
                #EXCLUDE_CINEMIZER_SDK
        INCLUDE_DIRECTORIES
            PUBLIC
                .
                ../Common
                ..
                ../..
                ../../Common
                Specification
                ${pal_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzFramework
                AZ::HLSLcc.Headers
                3rdParty::lz4
                3rdParty::glad
                Legacy::CryCommon
    )
    ly_add_source_properties(
        SOURCES
            ../../Common/Shaders/RemoteCompiler.cpp
            ../../Common/Textures/Image/DDSImage.cpp
        PROPERTY COMPILE_DEFINITIONS 
        VALUES ${LY_PAL_TOOLS_DEFINES}
    )

    ly_add_target(
        NAME CryRenderGL ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            opengl_renderer_shared_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryRenderGL.Static
    )

    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

        ly_add_target(
            NAME CryRenderGL.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Legacy
            FILES_CMAKE
                opengl_test_files.cmake
            PLATFORM_INCLUDE_FILES
                ${pal_dir}/DXGL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Legacy::CryRenderGL.Static
        )
        # SPEC-1548: disabling since it fails in mac with memory issues
        #ly_add_googletest(
        #    NAME Legacy::CryRenderGL.Tests
        #)
    endif()
endif()
