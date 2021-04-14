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

ly_add_target(
    NAME ResourceCompiler.Static STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        resourcecompiler_static_files.cmake
        Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ..
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Qt::Widgets
            Legacy::CryCommonTools
            Legacy::CryCommon
            Legacy::CryCommonTools
            Legacy::CryXML
            AZ::AzToolsFramework
    RUNTIME_DEPENDENCIES
        Legacy::CryXML
)


ly_add_target(
    NAME RC EXECUTABLE
    NAMESPACE Legacy
    OUTPUT_NAME rc
    FILES_CMAKE
        resourcecompiler_files.cmake
        Platform/Common/${PAL_TRAIT_COMPILER_ID}/rc_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Platform/${PAL_PLATFORM_NAME}
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::ResourceCompiler.Static
)
ly_add_source_properties(
    SOURCES ResourceCompiler.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

set(file_targets RC)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME RC.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            resourcecompiler_test_files.cmake
            Platform/Common/${PAL_TRAIT_COMPILER_ID}/rc_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::ResourceCompiler.Static
                AZ::AzFrameworkTestShared
    )
    ly_add_googletest(
        NAME Legacy::RC.Tests
    )
    list(APPEND file_targets RC.Tests)
endif()

ly_add_target_files(
    TARGETS 
        ${file_targets}
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../Config/rc/xmlfilter.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/../Config/rc/rc.ini
)
