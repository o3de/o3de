#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_pytest(
        NAME AutomatedTesting::ParallelEditorTestsWindows
        TEST_SUITE main
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main.py
        PYTEST_MARKS "SUITE_main"
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            Legacy::Editor
            AutomatedTesting.Assets
        COMPONENT
            TestTools
    )

endif()
