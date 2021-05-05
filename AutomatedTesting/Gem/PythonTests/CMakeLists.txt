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

################################################################################
# Automated Tests
################################################################################

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

add_subdirectory(assetpipeline)
add_subdirectory(atom_renderer)

## Physics ##
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::PhysicsTests_Main
        TEST_SUITE main
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/physics/TestSuite_Main.py
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            Physics
    )
    ly_add_pytest(
        NAME AutomatedTesting::PhysicsTests_Periodic
        TEST_SUITE periodic
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/physics/TestSuite_Periodic.py
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            Physics
    )
    ly_add_pytest(
        NAME AutomatedTesting::PhysicsTests_Sandbox
        TEST_SUITE sandbox
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/physics/TestSuite_Sandbox.py
        TIMEOUT 3600
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            Physics
    )
endif()

## ScriptCanvas ##
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::ScriptCanvasTests
        TEST_SUITE periodic
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/scripting/TestSuite_Active.py
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            ScriptCanvas
    )
    ly_add_pytest(
        NAME AutomatedTesting::ScriptCanvasTests_Sandbox
        TEST_SUITE sandbox
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/scripting/TestSuite_Sandbox.py
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
    )
endif()

## White Box ##
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::WhiteBoxTests
        TEST_SUITE main
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/WhiteBox/TestSuite_Active.py
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            WhiteBox
    )
endif()

## NvCloth ##
# [TODO LYN-1928] Enable when AutomatedTesting runs with Atom
#if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
#    ly_add_pytest(
#        NAME AutomatedTesting::NvClothTests
#        TEST_SUITE main
#        TEST_SERIAL
#        PATH ${CMAKE_CURRENT_LIST_DIR}/NvCloth/TestSuite_Active.py
#        TIMEOUT 1500
#        RUNTIME_DEPENDENCIES
#            Legacy::Editor
#            AZ::AssetProcessor
#            AutomatedTesting.Assets
#    )
#endif()

## Editor Python Bindings ##
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::EditorPythonBindings
        TEST_SUITE sandbox
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/EditorPythonBindings
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
            Gem::EditorPythonBindings.Editor
        COMPONENT TestTools
    )
endif()

## Python Asset Builder ##
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::PythonAssetBuilder
        TEST_SUITE periodic
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/PythonAssetBuilder
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
            Gem::EditorPythonBindings.Editor
            Gem::PythonAssetBuilder.Editor
        COMPONENT TestTools
    )
endif()

## Blast ##
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::BlastTests
        TEST_SUITE main
        TEST_SERIAL TRUE
        PATH ${CMAKE_CURRENT_LIST_DIR}/Blast/TestSuite_Active.py
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT Blast
    )
endif()

#############

## Large Worlds ##

include(${pal_dir}/PAL_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_LARGE_WORLDS_TEST_SUPPORTED)

## DynVeg ##
    ly_add_pytest(
        NAME AutomatedTesting::DynamicVegetationTests_Main
        TEST_SERIAL
        TEST_SUITE main
        PATH ${CMAKE_CURRENT_LIST_DIR}/largeworlds/dyn_veg
        PYTEST_MARKS "not SUITE_sandbox and not SUITE_periodic and not SUITE_benchmark"
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            Legacy::Editor
            AutomatedTesting.GameLauncher
            AutomatedTesting.Assets
        COMPONENT
            LargeWorlds
    )

    ly_add_pytest(
        NAME AutomatedTesting::DynamicVegetationTests_Sandbox
        TEST_SERIAL
        TEST_SUITE sandbox
        PATH ${CMAKE_CURRENT_LIST_DIR}/largeworlds/dyn_veg
        PYTEST_MARKS "SUITE_sandbox"
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            Legacy::Editor
            AutomatedTesting.GameLauncher
            AutomatedTesting.Assets
        COMPONENT
            LargeWorlds
    )

    ly_add_pytest(
        NAME AutomatedTesting::DynamicVegetationTests_Periodic
        TEST_SERIAL
        TEST_SUITE periodic
        PATH ${CMAKE_CURRENT_LIST_DIR}/largeworlds/dyn_veg
        PYTEST_MARKS "SUITE_periodic"
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            Legacy::Editor
            AutomatedTesting.Assets
        COMPONENT
            LargeWorlds
    )

## LandscapeCanvas ##
    ly_add_pytest(
        NAME AutomatedTesting::LandscapeCanvasTests_Main
        TEST_SERIAL
        TEST_SUITE main
        PATH ${CMAKE_CURRENT_LIST_DIR}/largeworlds/landscape_canvas
        PYTEST_MARKS "not SUITE_sandbox and not SUITE_periodic and not SUITE_benchmark"
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
           AZ::AssetProcessor
           Legacy::Editor
           AutomatedTesting.Assets
        COMPONENT
            LargeWorlds
    )

    ly_add_pytest(
        NAME AutomatedTesting::LandscapeCanvasTests_Periodic
        TEST_SERIAL
        TEST_SUITE periodic
        PATH ${CMAKE_CURRENT_LIST_DIR}/largeworlds/landscape_canvas
        PYTEST_MARKS "SUITE_periodic"
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
           AZ::AssetProcessor
           Legacy::Editor
           AutomatedTesting.Assets
        COMPONENT
            LargeWorlds
    )

## GradientSignal ##
    ly_add_pytest(
        NAME AutomatedTesting::GradientSignalTests_Periodic
        TEST_SERIAL
        TEST_SUITE periodic
        PATH ${CMAKE_CURRENT_LIST_DIR}/largeworlds/gradient_signal
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            Legacy::Editor
            AutomatedTesting.Assets
        COMPONENT
            LargeWorlds
    )

endif()

## Editor ##
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_FOUNDATION_TEST_SUPPORTED)
    ly_add_pytest(
        NAME AutomatedTesting::EditorTests_Periodic
        TEST_SUITE periodic
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/editor
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            Editor
    )
endif()

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    # Unstable, SPEC-3838 will restore
    #ly_add_pytest(
    #    NAME AutomatedTesting::asset_load_benchmark_test
    #    TEST_SERIAL
    #    TEST_SUITE benchmark
    #    PATH ${CMAKE_CURRENT_LIST_DIR}/streaming/benchmark/asset_load_benchmark_test.py
    #    RUNTIME_DEPENDENCIES
    #        AZ::AssetProcessor
    #        AZ::AssetProcessorBatch
    #        AutomatedTesting.GameLauncher
    #)
endif()

## Smoke ##
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::SmokeTest_Periodic
        TEST_SUITE periodic
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/smoke
        TIMEOUT 3600
        RUNTIME_DEPENDENCIES
	        AZ::AssetProcessor
            AZ::PythonBindingsExample
            Legacy::Editor
            AutomatedTesting.GameLauncher
            AutomatedTesting.Assets

