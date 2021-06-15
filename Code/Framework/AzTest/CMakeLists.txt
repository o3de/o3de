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

if(NOT LY_MONOLITHIC_GAME)
    ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/AzTest/Platform/${PAL_PLATFORM_NAME})

    ly_add_target(
        NAME AzTest STATIC
        NAMESPACE AZ
        FILES_CMAKE
            AzTest/aztest_files.cmake
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                .
                ${pal_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::googletest::GMock
                3rdParty::googletest::GTest
                3rdParty::GoogleBenchmark
                AZ::AzCore
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    )
endif()
