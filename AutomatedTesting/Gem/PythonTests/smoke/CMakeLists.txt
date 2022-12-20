#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)

    set(SMOKETEST_FILTER "SUITE_smoke")

    ly_add_pytest(
        NAME AutomatedTesting::SmokeTest
        TEST_SUITE smoke
        PATH ${CMAKE_CURRENT_LIST_DIR}
        PYTEST_MARKS ${SMOKETEST_FILTER}
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            AZ::PythonBindingsExample
            Legacy::Editor
            AutomatedTesting.GameLauncher
            AutomatedTesting.Assets
            AZ::AzTestRunner
            AZ::AssetBundlerBatch
            ${additional_dependencies}
        COMPONENT
            Smoke
    )

    ly_add_pytest(
        NAME AutomatedTesting::SmokeTest_Sandbox
        TEST_SUITE sandbox
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}
        PYTEST_MARKS "SUITE_sandbox"
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
    )

endif()
