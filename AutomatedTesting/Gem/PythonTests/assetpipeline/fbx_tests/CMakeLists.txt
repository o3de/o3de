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
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
        LABELS REQUIRES_tiaf
    )

    ly_add_pytest(
        NAME SceneProcessingTests.ScriptingTests
        PATH ${CMAKE_CURRENT_LIST_DIR}/scene_scripting_tests.py
        TEST_SUITE sandbox
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
    )
    
    ly_add_pytest(
        NAME AssetPipelineTests.Fbx_Tests
        PATH ${CMAKE_CURRENT_LIST_DIR}/fbx_tests.py
        TEST_SUITE sandbox
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessorBatch
            AZ::AssetProcessor
    )
endif()
