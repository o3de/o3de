#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
## AP Python Tests ##

add_subdirectory(asset_processor_tests)
add_subdirectory(fbx_tests)
add_subdirectory(scene_settings_tests)
add_subdirectory(scene_tests)
add_subdirectory(asset_bundler_tests)
add_subdirectory(metadata_relocation_tests)

    ly_add_pytest(
        NAME AssetPipelineTests.Periodic
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Periodic.py
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            AutomatedTesting.Assets
            Legacy::Editor
        COMPONENT
            Atom
    )

    ly_add_pytest(
        NAME AssetPipelineTests.BankInfoParser
        PATH ${CMAKE_CURRENT_LIST_DIR}/wwise_bank_dependency_tests/bank_info_parser_tests.py
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        TEST_SUITE periodic
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
    )
   
endif()

