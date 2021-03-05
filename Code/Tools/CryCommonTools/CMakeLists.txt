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

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME CryCommonTools STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        crycommontools_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::lz4
            3rdParty::zlib
            3rdParty::zstd
            AZ::AzCore
        PUBLIC
            Legacy::CryCommon
            AZ::AzFramework
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME CryCommonTools.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            crycommontools_tests_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                UnitTests
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommonTools
                AZ::AzTest
    )
    ly_add_googletest(
        NAME Legacy::CryCommonTools.Tests
    )
endif()
