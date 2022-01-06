#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
## AP Python Tests ##

    ly_add_pytest(
        NAME AssetPipelineTests.Batch
        TEST_SUITE sandbox
        PATH ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        RUNTIME_DEPENDENCIES       
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )
    
    ly_add_pytest(
        NAME AssetPipelineTests.Batch_2_Main
        TEST_SUITE main
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_tests_2.py
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.Batch_2_Sandbox
        TEST_SUITE sandbox
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_tests_2.py
        PYTEST_MARKS "SUITE_sandbox" # run only sandbox tests in this file
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.BatchDependency
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_dependency_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.BatchDependency_Sandbox
        TEST_SUITE sandbox
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_batch_dependency_tests2.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )
    
    ly_add_pytest(
        NAME AssetPipelineTests.Gui
        TEST_SUITE sandbox
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_gui_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.Gui_2_Main
        TEST_SUITE main
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_gui_tests_2.py
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

    ly_add_pytest(
        NAME AssetPipelineTests.Gui_2_Sandbox
        TEST_SUITE sandbox
        PATH  ${CMAKE_CURRENT_LIST_DIR}/asset_processor_gui_tests_2.py
        PYTEST_MARKS "SUITE_sandbox" # run only sandbox tests in this file
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )

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

    ly_add_pytest(
        NAME AssetPipelineTests.AssetBuilder
        PATH ${CMAKE_CURRENT_LIST_DIR}/asset_builder_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SERIAL 
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
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
        
endif()
