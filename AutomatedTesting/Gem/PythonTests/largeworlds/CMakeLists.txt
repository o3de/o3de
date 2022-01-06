#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_LARGE_WORLDS_TEST_SUPPORTED)

## DynVeg ##

    ly_add_pytest(
        NAME AutomatedTesting::DynamicVegetationTests_Main_Optimized
        TEST_SERIAL
        TEST_SUITE main
        PATH ${CMAKE_CURRENT_LIST_DIR}/dyn_veg/TestSuite_Main_Optimized.py
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            Legacy::Editor
            AutomatedTesting.Assets
            AutomatedTesting.GameLauncher
        COMPONENT
            LargeWorlds
    )
    ly_add_pytest(
        NAME AutomatedTesting::DynamicVegetationTests_Periodic_Optimized
        TEST_SERIAL
        TEST_SUITE periodic
        PATH ${CMAKE_CURRENT_LIST_DIR}/dyn_veg/TestSuite_Periodic_Optimized.py
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            Legacy::Editor
            AutomatedTesting.Assets
            AutomatedTesting.GameLauncher
        COMPONENT
            LargeWorlds
    )

## LandscapeCanvas ##

    ly_add_pytest(
        NAME AutomatedTesting::LandscapeCanvasTests_Main_Optimized
        TEST_SERIAL
        TEST_SUITE main
        PATH ${CMAKE_CURRENT_LIST_DIR}/landscape_canvas/TestSuite_Main_Optimized.py
        RUNTIME_DEPENDENCIES
           AZ::AssetProcessor
           Legacy::Editor
           AutomatedTesting.Assets
        COMPONENT
            LargeWorlds
    )

## GradientSignal ##

    ly_add_pytest(
        NAME AutomatedTesting::GradientSignalTests_Periodic_Optimized
        TEST_SERIAL
        TEST_SUITE periodic
        PATH ${CMAKE_CURRENT_LIST_DIR}/gradient_signal/TestSuite_Periodic_Optimized.py
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            Legacy::Editor
            AutomatedTesting.Assets
        COMPONENT
            LargeWorlds
    )

endif()
