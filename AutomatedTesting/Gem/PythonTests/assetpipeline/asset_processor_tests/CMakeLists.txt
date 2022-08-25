#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
## AP Python Tests ##

## Main Tests ##

    ly_add_pytest(
        NAME AssetPipelineTests.Batch_2_Main
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_tests_2.py        
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE main
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.ScriptCanvasAssetProcessorTests_Main
        PATH ${CMAKE_CURRENT_LIST_DIR}/ScriptCanvasAssetProcessorTests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE main
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        RUNTIME_DEPENDENCIES       
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
        COMPONENT
            ScriptCanvas
    )

## Periodic Tests ##

    ly_add_pytest(
        NAME AssetPipelineTests.AssetBuilder_Periodic
        PATH ${CMAKE_CURRENT_LIST_DIR}/asset_builder_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SERIAL 
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.Batch_Periodic
        PATH ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE periodic        
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        RUNTIME_DEPENDENCIES       
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )
    
    ly_add_pytest(
        NAME AssetPipelineTests.Batch_Periodic_Serial
        PATH ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_periodic_serial_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SERIAL
        TEST_SUITE periodic        
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        RUNTIME_DEPENDENCIES       
            AZ::AssetProcessorBatch
    )

    ly_add_pytest(
        NAME AssetPipelineTests.BatchDependency_Periodic
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_dependency_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE periodic
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.BatchDependency_2_Periodic
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_dependency_tests2.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.Gui_Periodic
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_gui_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE periodic
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.Gui_2_Periodic
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_gui_tests_2.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE        
        TEST_SUITE periodic
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.AssetRelocator_Periodic
        PATH ${CMAKE_CURRENT_LIST_DIR}/asset_relocator_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE periodic
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
    )

    ly_add_pytest(
        NAME AssetProcessorCacheServerTests.Periodic
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_cache_server.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE periodic
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
    )

## Sandbox Tests ##

    ly_add_pytest(
        NAME AssetPipelineTests.Batch_Sandbox
        PATH ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE sandbox
        PYTEST_MARKS "SUITE_sandbox" # run only sandbox tests in this file
        RUNTIME_DEPENDENCIES       
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

## Non-Linux Tests ##

    set(SUPPORTED_PLATFORMS "Windows" "Mac")
    if (NOT "${PAL_PLATFORM_NAME}" IN_LIST SUPPORTED_PLATFORMS)
        return()
    endif()

## Periodic Tests ##

    ly_add_pytest(
        NAME AssetPipelineTests.AssetBundler
        PATH ${CMAKE_CURRENT_LIST_DIR}/asset_bundler_batch_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SERIAL 
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            AZ::AssetBundlerBatch
    )

    ly_add_pytest(
        NAME AssetPipelineTests.MissingDependency
        PATH ${CMAKE_CURRENT_LIST_DIR}/missing_dependency_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SERIAL 
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
    )

    ly_add_pytest(
        NAME AssetPipelineTests.BundleMode
        PATH ${CMAKE_CURRENT_LIST_DIR}/bundle_mode_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SERIAL
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            AZ::AssetBundlerBatch
            Legacy::Editor
            AutomatedTesting.Assets
    )

endif()
