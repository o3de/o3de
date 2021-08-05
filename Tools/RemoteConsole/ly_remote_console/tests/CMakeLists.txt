#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# Ly Remote Console tests.
#

# Unit tests.
ly_add_pytest(
    NAME RemoteConsole_UnitTests_main_no_gpu
    PATH ${CMAKE_CURRENT_LIST_DIR}/unit/
)

get_property(LY_PROJECTS_TARGET_NAME GLOBAL PROPERTY LY_PROJECTS_TARGET_NAME)
if(PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_BUILD_TESTS_SUPPORTED AND AutomatedTesting IN_LIST LY_PROJECTS_TARGET_NAME)
    # Integration tests.
#    ly_add_pytest(
#        NAME RemoteConsole_IntegTests_periodic_no_gpu
#        PATH ${CMAKE_CURRENT_LIST_DIR}/integ/test_remote_console.py
#        TEST_SERIAL
#        TEST_SUITE periodic
#        RUNTIME_DEPENDENCIES
#            Legacy::Editor
#            AssetProcessor
#            AutomatedTesting.GameLauncher
#            AutomatedTesting.Assets
#    )
endif()