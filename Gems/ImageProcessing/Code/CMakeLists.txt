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

ly_add_target(
    NAME ImageProcessing.Headers HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        imageprocessing_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
)

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

set(platform_tools_files)
set(pal_tools_include_files)
set(pal_tools_dirs)
foreach(enabled_platform ${LY_PAL_TOOLS_ENABLED})
    string(TOLOWER ${enabled_platform} enabled_platform_lowercase)
    ly_get_list_relative_pal_filename(pal_tools_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${enabled_platform})
    list(APPEND platform_tools_files ${pal_tools_dir}/pal_tools_${enabled_platform_lowercase}.cmake)
    list(APPEND pal_tools_include_files ${pal_tools_dir}/pal_tools_${enabled_platform_lowercase}_files.cmake)
    list(APPEND pal_tools_dirs ${pal_tools_dir})
endforeach()

ly_add_target(
    NAME ImageProcessing.Static STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTOUIC
    FILES_CMAKE
        imageprocessing_static_files.cmake
        ${pal_tools_include_files}
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${platform_tools_files}
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            ../External
            ${pal_dir}
            ${pal_tools_dirs}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Widgets
            3rdParty::etc2comp
            3rdParty::PVRTexTool
            3rdParty::squish-ccr
            3rdParty::tiff
            Legacy::CryCommon
            AZ::AzCore
            AZ::AssetBuilderSDK
            Gem::TextureAtlas
)
ly_add_source_properties(
    SOURCES
        Source/BuilderSettings/BuilderSettingManager.cpp
        Source/Processing/ImageConvert.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

ly_add_target(
    NAME ImageProcessing.Editor MODULE
    NAMESPACE Gem
    OUTPUT_NAME Gem.ImageProcessing.Editor.eeffbd9211cf4ce0b5cc73696b427cbe.v0.1.0
    AUTOMOC
    AUTORCC
    FILES_CMAKE
        imageprocessing_files.cmake
    PLATFORM_INCLUDE_FILES
        ${platform_tools_files}
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            ${pal_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Widgets
            3rdParty::PVRTexTool
            3rdParty::squish-ccr
            Legacy::CryCommon
            AZ::AzCore
            AZ::AzToolsFramework
            AZ::AssetBuilderSDK
            Gem::ImageProcessing.Static
            Gem::TextureAtlas
    RUNTIME_DEPENDENCIES
        3rdParty::ASTCEncoder
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ImageProcessing.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            imageprocessing_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                .
                Source
                ${pal_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                3rdParty::Qt::Widgets
                Legacy::CryCommon
                AZ::AssetBuilderSDK
                Gem::ImageProcessing.Static
                Gem::ImageProcessing.Editor
                Gem::TextureAtlas
    )
    ly_add_googletest(
        NAME Gem::ImageProcessing.Tests
    )

    ly_add_source_properties(
        SOURCES Tests/ImageProcessing_Test.cpp
        PROPERTY COMPILE_DEFINITIONS 
        VALUES ${LY_PAL_TOOLS_DEFINES}
    )

endif()
