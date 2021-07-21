#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_FOUNDATION_TEST_SUPPORTED)
    ly_add_pytest(
        NAME AutomatedTesting::EditorTests_Main
        TEST_SUITE main
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}
        PYTEST_MARKS "SUITE_main and not REQUIRES_gpu"
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            Editor
    )

    ly_add_pytest(
        NAME AutomatedTesting::EditorTests_Periodic
        TEST_SUITE periodic
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}
        PYTEST_MARKS "SUITE_periodic and not REQUIRES_gpu"
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            Editor
    )

    ly_add_pytest(
        NAME AutomatedTesting::EditorTests_Main_GPU
        TEST_SUITE main
        TEST_SERIAL
        TEST_REQUIRES gpu
        PATH ${CMAKE_CURRENT_LIST_DIR}
        PYTEST_MARKS "SUITE_main and REQUIRES_gpu"
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            Editor
    )

    ly_add_pytest(
        NAME AutomatedTesting::EditorTests_Sandbox
        TEST_SUITE sandbox
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}
        PYTEST_MARKS "SUITE_sandbox"
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            Editor
    )
endif()
