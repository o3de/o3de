#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME SceneProcessingTests.PythonAssetBuilderTests
        TEST_SUITE main
        PATH  ${CMAKE_CURRENT_LIST_DIR}/pythonassetbuildertests.py
        PYTEST_MARKS "not SUITE_sandbox" # don't run sandbox tests in this file
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )
    

    ly_add_pytest(
        NAME AssetPipelineTests.Fbx_Tests
        PATH ${CMAKE_CURRENT_LIST_DIR}/fbx_test/fbx_test.py
        TEST_SUITE sandbox
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )
endif()
