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
    NAME AssetBundler.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        assetbundlerbatch_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    COMPILE_DEFINITIONS
        PRIVATE
            METRICS_ENABLED
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzToolsFramework
            ${additional_dependencies}
)

ly_add_target(
    NAME AssetBundlerBatch EXECUTABLE
    NAMESPACE AZ
    FILES_CMAKE
        assetbundlerbatch_exe_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AssetBundler.Static
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME AssetBundler.Tests EXECUTABLE
        NAMESPACE AZ
        FILES_CMAKE
            assetbundlerbatch_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AssetBundler.Static
                AZ::AzFrameworkTestShared
    )

    ly_add_googletest(
        NAME AZ::AssetBundler.Tests
        TEST_COMMAND $<TARGET_FILE:AZ::AssetBundler.Tests> --unittest
    )

endif()
