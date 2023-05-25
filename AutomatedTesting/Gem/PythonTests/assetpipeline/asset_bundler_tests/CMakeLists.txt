#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
## AP Python Tests ##

## Periodic Tests ##

    ly_add_pytest(
        NAME APTests.AssetBundler
        PATH ${CMAKE_CURRENT_LIST_DIR}/asset_bundler_batch_tests.py
        TEST_SERIAL 
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            AZ::AssetBundlerBatch
            AutomatedTesting.Assets
    )

## Non-Linux Tests ##

    set(SUPPORTED_PLATFORMS "Windows" "Mac")
    if (NOT "${PAL_PLATFORM_NAME}" IN_LIST SUPPORTED_PLATFORMS)
        return()
    endif()

## Periodic Tests ##

    ly_add_pytest(
        NAME APTests.BundleMode
        PATH ${CMAKE_CURRENT_LIST_DIR}/bundle_mode_tests.py
        TEST_SERIAL
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            AZ::AssetBundlerBatch
            Legacy::Editor
            AutomatedTesting.Assets
    )

endif()
