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

include(${pal_dir}/DXMETAL_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(DXGL_TRAIT_BUILD_DXMETAL_SUPPORTED AND NOT LY_MONOLITHIC_GAME) # Only Atom is supported in monolithic builds
    ly_add_target(
        NAME CryRenderMetal.Static STATIC
        NAMESPACE Legacy
        FILES_CMAKE
            metal_renderer_files.cmake
            metal_mesh_baker_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/DXMETAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                DO_RENDERLOG
                DO_RENDERSTATS
                DIRECT3D10
                XRENDERGL_EXPORTS
                _RENDERER
                OPENGL
                CRY_USE_METAL
        INCLUDE_DIRECTORIES
            PUBLIC
                ../..
                .
                ..
                ../../Common
                ${pal_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzFramework
                Legacy::CryCommon
                Legacy::CryCommon.EngineSettings.Static
    )

    ly_add_target(
        NAME CryRenderMetal ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            metal_renderer_shared_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/DXMETAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryRenderMetal.Static
    )

    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        ly_add_target(
            NAME CryRenderMetal.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Legacy
            FILES_CMAKE
                metal_renderer_shared_files.cmake
                metal_test_files.cmake
            PLATFORM_INCLUDE_FILES
                ${pal_dir}/DXMETAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Legacy::CryRenderMetal.Static
        )
        # SPEC-1548: disabling since it fails in mac with memory issues
        #ly_add_googletest(
        #    NAME Legacy::CryRenderMetal.Tests
        #)
    endif()
endif()
