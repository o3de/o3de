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

ly_add_target(
    NAME GridMate STATIC
    NAMESPACE AZ
    FILES_CMAKE
        GridMate/gridmate_files.cmake
        GridMate/gridmate_ssl_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${common_dir}/gridmate_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::OpenSSL
            AZ::AzCore
)

ly_add_source_properties(
    SOURCES GridMate/Session/Session.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    
    ly_get_list_relative_pal_filename(pal_test_dir ${CMAKE_CURRENT_LIST_DIR}/Tests/Platform/${PAL_PLATFORM_NAME})
    ly_add_target(
        NAME GridMate.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            Tests/gridmate_test_files.cmake
            ${pal_test_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                ${pal_test_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::GridMate
                AZ::AzTest
    )
    ly_add_googletest(
        NAME AZ::GridMate.Tests
    )
endif()

