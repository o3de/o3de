#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ImageProcessingAtom.Headers HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        imageprocessingatom_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
)

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

set(platform_tools_files)
set(pal_tools_include_files)
set(pal_tools_include_dirs)
foreach(enabled_platform ${LY_PAL_TOOLS_ENABLED})
    string(TOLOWER ${enabled_platform} enabled_platform_lowercase)

    ly_get_list_relative_pal_filename(pal_tools_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${enabled_platform})
    list(APPEND pal_tools_include_dirs ${pal_tools_source_dir})

    list(APPEND platform_tools_files ${pal_tools_source_dir}/pal_tools_${enabled_platform_lowercase}.cmake)
    list(APPEND pal_tools_include_files ${pal_tools_source_dir}/pal_tools_${enabled_platform_lowercase}_files.cmake)
endforeach()

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

ly_add_target(
    NAME ImageProcessingAtom.Editor.Static STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTOUIC
    FILES_CMAKE
        imageprocessing_files.cmake
        ${pal_tools_include_files}
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${common_source_dir}/${PAL_TRAIT_COMPILER_ID}/imageprocessingatom_editor_static_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        ${platform_tools_files}
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            ${pal_source_dir}
            ${pal_tools_include_dirs}
            .
            Source
            ../External/CubeMapGen
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Qt::Core
            3rdParty::Qt::Widgets
            3rdParty::Qt::Gui
            3rdParty::astc-encoder
            3rdParty::squish-ccr
            3rdParty::TIFF
            3rdParty::ISPCTexComp
            3rdParty::ilmbase
            AZ::AzFramework
            AZ::AzToolsFramework
            AZ::AzQtComponents
            AZ::AssetBuilderSDK
            AZ::AtomCore
            Gem::Atom_RPI.Public
            Gem::Atom_RHI.Reflect
            Gem::Atom_Utils.Static

)
ly_add_source_properties(
    SOURCES
        Source/Processing/ImageConvert.cpp
        Source/BuilderSettings/BuilderSettingManager.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

ly_add_target(
    NAME ImageProcessingAtom.Editor GEM_MODULE

    NAMESPACE Gem
    FILES_CMAKE
        imageprocessingatom_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::ImageProcessingAtom.Editor.Static
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ImageProcessingAtom.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            imageprocessing_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::ImageProcessingAtom.Editor.Static
    )
    ly_add_googletest(
        NAME Gem::ImageProcessingAtom.Editor.Tests
    )
endif()
