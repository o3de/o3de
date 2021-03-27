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

set(LY_RENDER_TO_TEXTURE OFF CACHE BOOL "If True, sets render to texture global define AZ_RENDER_TO_TEXTURE_GEM_ENABLED=1. Required to use Render to Texture gem")

# TODO Fix global define https://jira.agscollab.com/browse/LY-116363
if(LY_RENDER_TO_TEXTURE)
    add_definitions(-DAZ_RENDER_TO_TEXTURE_GEM_ENABLED=1)
endif()

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(tests_common_dir ${CMAKE_CURRENT_LIST_DIR}/Tests/Platform/Common)

ly_add_target(
    NAME RenderToTexture.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        rendertotexture_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Source
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
            Gem::LmbrCentral
    RUNTIME_DEPENDENCIES
        Legacy::Cry3DEngine
)

ly_add_target(
    NAME RenderToTexture ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        rendertotexture_shared_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::RenderToTexture.Static
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    
    ly_add_target(
        NAME RenderToTexture.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            rendertotexture_files.cmake
            rendertotexture_editor_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                Source
                Include
        COMPILE_DEFINITIONS
            PUBLIC
                RENDER_TO_TEXTURE_EDITOR=1
        BUILD_DEPENDENCIES
            PUBLIC
                Legacy::CryCommon
                Gem::LmbrCentral
                Legacy::EditorCore
                Legacy::EditorCommon
                AZ::AzToolsFramework
    )

    ly_add_target(
        NAME RenderToTexture.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            rendertotexture_shared_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::RenderToTexture.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND LY_RENDER_TO_TEXTURE) # Test targets rely on AZ_RENDER_TO_TEXTURE_GEM_ENABLED define.
    ly_add_target(
        NAME RenderToTexture.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            rendertotexture_tests_files.cmake
            rendertotexture_shared_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzCore
                Gem::RenderToTexture.Static
    )
    ly_add_googletest(
        NAME Gem::RenderToTexture.Tests
    )

    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME RenderToTexture.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                rendertotexture_tests_files.cmake
                rendertotexture_shared_files.cmake
            PLATFORM_INCLUDE_FILES
                ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Include
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzCore
                    Gem::RenderToTexture.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::RenderToTexture.Editor.Tests
        )
    endif()
endif()
