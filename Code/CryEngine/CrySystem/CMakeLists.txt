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
ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)
add_subdirectory(XML)

# The following target is a 'C' file only library to work around an issue in cmake and VS generators that
# will append 'std=c++17' to both C and C++ compiler flags for clang. Do not add any .cpp files to this
# library.
ly_add_target(
    NAME CrySystem.DLMalloc.C STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        crysystem_dlmalloc_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
        PRIVATE
            ${pal_dir}
)


ly_get_pal_tool_dirs(pal_tool_dirs ${CMAKE_CURRENT_LIST_DIR}/Platform)

ly_add_target(
    NAME CrySystem.Static STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        crysystem_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
        PRIVATE
            ${common_dir}
            ${pal_tool_dirs}
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CrySystem.DLMalloc.C
        PRIVATE
            3rdParty::expat
            3rdParty::lz4
            3rdParty::md5
            3rdParty::tiff
            3rdParty::zlib
            3rdParty::zstd
            Legacy::CryCommon
            Legacy::CrySystem.XMLBinary
            Legacy::RemoteConsoleCore
            AZ::AzFramework
            AZ::CrashHandler
    RUNTIME_DEPENDENCIES
        Legacy::Cry3DEngine
)

ly_add_source_properties(
    SOURCES SystemInit.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

ly_add_source_properties(
    SOURCES SystemCFG.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES LY_BUILD=${LY_VERSION_BUILD_NUMBER}
)

ly_add_target(
    NAME CrySystem ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE Legacy
    FILES_CMAKE
        crysystem_shared_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CrySystem.Static
            AZ::AzCore
            Legacy::CryCommon
            Legacy::CryCommon.EngineSettings.Static
    RUNTIME_DEPENDENCIES
        Legacy::CryFont
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME CrySystem.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            crysystem_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::CryCommon
                Legacy::CryCommon.EngineSettings.Static
                Legacy::CrySystem.Static
                AZ::AzFramework
    )
    ly_add_googletest(
        NAME Legacy::CrySystem.Tests
    )
endif()

