#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_pytest(
        NAME AutomatedTesting::Atom_TestSuite_Main
        TEST_SUITE main
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main.py
        TEST_SERIAL
        TIMEOUT 600
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AutomatedTesting.Assets
            Editor
        COMPONENT
            Atom
    )
    ly_add_pytest(
        NAME AutomatedTesting::Atom_TestSuite_Main_Optimized
        TEST_SUITE main
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main_Optimized.py
        TEST_SERIAL
        TIMEOUT 600
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AutomatedTesting.Assets
            Editor
        COMPONENT
            Atom
    )
    ly_add_pytest(
        NAME AutomatedTesting::Atom_TestSuite_Sandbox
        TEST_SUITE sandbox
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Sandbox.py
        TEST_SERIAL
        TIMEOUT 400
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AutomatedTesting.Assets
            Editor
        COMPONENT
            Atom
    )
    ly_add_pytest(
        NAME AutomatedTesting::Atom_TestSuite_Main_GPU
        TEST_SUITE main
        TEST_REQUIRES gpu
        TEST_SERIAL
        TIMEOUT 1200
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main_GPU.py
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AutomatedTesting.Assets
            Editor
        COMPONENT
            Atom
    )
    ly_add_pytest(
        NAME AutomatedTesting::Atom_TestSuite_Main_GPU_Optimized
        TEST_SUITE main
        TEST_REQUIRES gpu
        TEST_SERIAL
        TIMEOUT 1200
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main_GPU_Optimized.py
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AutomatedTesting.Assets
            Editor
        COMPONENT
            Atom
    )
endif()
