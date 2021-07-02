#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    # Unstable, SPEC-3838 will restore
    #ly_add_pytest(
    #    NAME AutomatedTesting::asset_load_benchmark_test
    #    TEST_SERIAL
    #    TEST_SUITE benchmark
    #    PATH ${CMAKE_CURRENT_LIST_DIR}/benchmark/asset_load_benchmark_test.py
    #    RUNTIME_DEPENDENCIES
    #        AZ::AssetProcessor
    #        AZ::AssetProcessorBatch
    #        AutomatedTesting.GameLauncher
    #)
endif()
