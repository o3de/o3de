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

ly_add_target(
    NAME CryNetwork.Static STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        crynetwork_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
            AZ::GridMate
)

ly_add_target(
    NAME CryNetwork ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE Legacy
    FILES_CMAKE
        crynetwork_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            ${pal_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryNetwork.Static
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME CryNetwork.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            crynetwork_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                .
                ${pal_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::CryNetwork.Static
    )
    ly_add_googletest(
        NAME Legacy::CryNetwork.Tests
    )
endif()

