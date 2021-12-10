#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

################################################################################
# AWS Automated Tests
# Runs AWS Gems automation tests.
################################################################################

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    # Only enable AWS automated tests on Windows
    set(SUPPORTED_PLATFORMS "Windows" "Linux")
    if (NOT "${PAL_PLATFORM_NAME}" IN_LIST SUPPORTED_PLATFORMS)
        return()
    endif()

    ly_add_pytest(
        NAME AutomatedTesting::AWSTests
        TEST_SUITE awsi
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/${PAL_PLATFORM_NAME}/
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            AutomatedTesting.GameLauncher
            AutomatedTesting.Assets
        COMPONENT
            AWS
    )
endif()
