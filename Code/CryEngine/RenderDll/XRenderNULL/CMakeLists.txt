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

include(${pal_dir}/NULLRenderer_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(CRYRENDER_TRAIT_BUILD_NULL_RENDERER_SUPPORTED)

    ly_add_target(
        NAME CryRenderNULL.Static STATIC
        NAMESPACE Legacy
        FILES_CMAKE
            core_null_renderer_files.cmake
            ${pal_dir}/core_null_renderer_files_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                NULL_RENDERER
                _RENDERER
        INCLUDE_DIRECTORIES
            PUBLIC
                .
                ..
                ../Common
                ../XRenderD3D9
                ${pal_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::LZSS
                3rdParty::Lzma
                Legacy::CryCommon
                Legacy::CryCommon.EngineSettings.Static
    )

    if (NOT LY_MONOLITHIC_GAME) # Only Atom is supported in monolithic builds
        ly_add_target(
            NAME CryRenderNULL ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
            NAMESPACE Legacy
            FILES_CMAKE
                core_null_renderer_shared_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
            BUILD_DEPENDENCIES
                PRIVATE
                    Legacy::CryRenderNULL.Static
        )

        if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

            ly_add_target(
                NAME CryRenderNULL.Tests MODULE
                NAMESPACE Legacy
                FILES_CMAKE
                    core_null_renderer_test_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        .
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Legacy::CryRenderNULL.Static
            )
            # SPEC-1548: disabling since it fails in mac with memory issues
            #ly_add_googletest(
            #    NAME Legacy::CryRenderNULL.Tests
            #)
        endif()
    endif()

endif()
