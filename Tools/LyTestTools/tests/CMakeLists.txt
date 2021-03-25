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
# LyTestTools tests.
#

# Unit tests.
ly_add_pytest(
    NAME LyTestTools_UnitTests
    TEST_SUITE smoke
    PATH ${CMAKE_CURRENT_LIST_DIR}/unit/
    COMPONENT TestTools
)

if(PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_BUILD_TESTS_SUPPORTED AND AutomatedTesting IN_LIST LY_PROJECTS_NAME)
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
            Legacy::CryRenderNULL
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
        NAME LyTestTools_IntegTests_Settings_smoke_no_gpu
        PATH ${CMAKE_CURRENT_LIST_DIR}/integ/test_settings.py
        TEST_SERIAL
        TEST_SUITE smoke
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AutomatedTesting.GameLauncher
            AutomatedTesting.Assets
            Legacy::CryRenderNULL
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
            Legacy::CryRenderNULL
            COMPONENT TestTools
    )
endif()
