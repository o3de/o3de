#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_BUILD_TESTS_SUPPORTED)
#    ly_add_pytest(
#        NAME AutomatedTesting::Atom_Main_Null_Render_00
#        TEST_SUITE main
#        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main_Null_Render_Level_Loads.py
#        TEST_SERIAL
#        TIMEOUT 600
#        RUNTIME_DEPENDENCIES
#            AssetProcessor
#            AutomatedTesting.Assets
#            Editor
#        COMPONENT
#            Atom
#    )
    ly_add_pytest(
        NAME AutomatedTesting::Atom_Main_Null_Render_01
        TEST_SUITE main
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main_Null_Render_Component_01.py
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
        NAME AutomatedTesting::Atom_Main_Null_Render_02
        TEST_SUITE main
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main_Null_Render_Component_02.py
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
        NAME AutomatedTesting::Atom_Main_Null_Render_03
        TEST_SUITE main
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main_Null_Render_Component_03.py
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
        NAME AutomatedTesting::Atom_TestSuite_Smoke_GPU
        TEST_SUITE smoke
        TEST_REQUIRES gpu
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Smoke_GPU.py
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
        NAME AutomatedTesting::Atom_TestSuite_Benchmark_GPU
        TEST_SUITE benchmark
        TEST_REQUIRES gpu
        TEST_SERIAL
        TIMEOUT 700
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Benchmark_GPU.py
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AutomatedTesting.Assets
            Editor
        COMPONENT
            Atom
    )
    ly_add_pytest(
        NAME AutomatedTesting::Atom_Main_Null_Render_MaterialEditor
        TEST_SUITE main
        TEST_SERIAL
        TIMEOUT 700
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main_Null_Render_MaterialEditor_01.py
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AutomatedTesting.Assets
            MaterialEditor
        COMPONENT
            Atom
    )
    ly_add_pytest(
        NAME AutomatedTesting::Atom_Main_Null_Render_MaterialCanvas
        TEST_SUITE main
        TEST_SERIAL
        TIMEOUT 700
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main_Null_Render_MaterialCanvas_01.py
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AutomatedTesting.Assets
            MaterialCanvas
        COMPONENT
            Atom
    )
    ly_add_pytest(
        NAME AutomatedTesting::Atom_Periodic_Null_Render_01
        TEST_SUITE periodic
        TEST_SERIAL
        TIMEOUT 1200
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Periodic_Null_Render_01.py
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AutomatedTesting.Assets
            Editor
            MaterialCanvas
            MaterialEditor
        COMPONENT
            Atom
    )
endif()
