#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# LyTestTools tests.
#

# Unit tests.
ly_add_pytest(
    NAME LyTestTools_UnitTests
    TEST_SUITE smoke
    PATH ${CMAKE_CURRENT_LIST_DIR}/unit/
    COMPONENT TestTools
)

get_property(LY_PROJECTS_TARGET_NAME GLOBAL PROPERTY LY_PROJECTS_TARGET_NAME)
if(PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_BUILD_TESTS_SUPPORTED AND AutomatedTesting IN_LIST LY_PROJECTS_TARGET_NAME)
    # Integration tests.
    ly_add_pytest(
        NAME LyTestTools_IntegTests_Sanity_smoke_no_gpu
        PATH ${CMAKE_CURRENT_LIST_DIR}/integ/sanity_tests.py
        TEST_SERIAL
        TEST_SUITE smoke
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AssetProcessor
            AutomatedTesting.GameLauncher
            AutomatedTesting.Assets
            COMPONENT TestTools
    )

    ly_add_pytest(
        NAME LyTestTools_IntegTests_ProcessUtils_smoke_no_gpu
        PATH ${CMAKE_CURRENT_LIST_DIR}/integ/test_process_utils.py
        TEST_SERIAL
        TEST_SUITE smoke
        COMPONENT TestTools
    )
    
    ly_add_pytest(
        NAME LyTestTools_IntegTests_ProcessUtilsLinux_smoke_no_gpu
        PATH ${CMAKE_CURRENT_LIST_DIR}/integ/test_process_utils_linux.py
        TEST_SERIAL
        TEST_SUITE smoke
        COMPONENT TestTools
    )

    # Regression tests.
    ly_add_pytest(
        NAME LyTestTools_IntegTests_RegressionTests_periodic_no_gpu
        PATH ${CMAKE_CURRENT_LIST_DIR}/integ/test_regression.py
        TEST_SERIAL
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AssetProcessor
            AutomatedTesting.GameLauncher
            AutomatedTesting.Assets
            COMPONENT TestTools
    )
endif()
