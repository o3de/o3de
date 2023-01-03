#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

## Prefab ##
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::PrefabTests_Main
        TEST_SUITE main
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main.py
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
    )

    ly_add_pytest(
        NAME AutomatedTesting::PrefabTests_Periodic
        TEST_SUITE periodic
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Periodic.py
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
    )

    ly_add_pytest(
        NAME AutomatedTesting::PrefabLauncherTests_Periodic
        TEST_SUITE periodic
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/LauncherTestSuite_Periodic.py
        RUNTIME_DEPENDENCIES
            AutomatedTesting.GameLauncher
            AZ::AssetProcessor
            AutomatedTesting.Assets
    )
endif()
